#!/usr/bin/env python3
"""Where Claude Code tokens went, and why: a deeper report than tools/token_usage.py.

  python -I tools/token_report.py <since> <until> <transcript folder> [...]
  e.g. python -I tools/token_report.py 2026-10-17T00:00 2026-10-25T00:00 ~/.claude/projects/C--Users-*-byo-pop*

Times are UTC (ISO, as in the transcripts). Give every project folder of the sessions to compare (a
game's worktree has its own); subagent and workflow transcripts below them are included. Calls copied
into a forked session's transcript are counted once.

Each API call re-reads its whole context, mostly as cache reads, so a piece of context costs its size
once per later call until the next compaction. The report charges each piece (tool results by kind,
the model's own tool inputs and text, reminders and attachments, the fixed prompt) with size x later
calls; what the estimates miss (thinking blocks, the tokenizer's real ratio) is its own line. It also
splits the calls into main sessions, subagents and workflow agents, finds the cache writes that rewrote
a whole context and why (a gap longer than the cache lifetime, a new agent, a compaction), and weighs
everything at Opus 5.5 API prices. docs/TOKEN_ECONOMY.md has what a run of it found and the rules
drawn from it.
"""
import collections, json, os, re, sys

if len(sys.argv) < 4:
    print(__doc__.strip())
    sys.exit(2)
since, until = sys.argv[1], sys.argv[2]
roots = sys.argv[3:]
CHARS_PER_TOKEN = 3.5
IMAGE_TOKENS = 1500

def tok(n):
    return int(n / CHARS_PER_TOKEN)

def text_of(content):
    if isinstance(content, str):
        return content, 0
    n, imgs = [], 0
    if isinstance(content, list):
        for b in content:
            if not isinstance(b, dict):
                continue
            if b.get('type') == 'text':
                n.append(b.get('text', ''))
            elif b.get('type') == 'image':
                imgs += 1
            elif b.get('type') == 'tool_reference':
                n.append(json.dumps(b))
    return '\n'.join(n), imgs

SEG = re.compile(r'\s*(?:&&|\|\||;|\|)\s*')
def bash_kind(cmd):
    c = cmd.strip()
    for key, pat in (('oracle', r'compare\.sh'), ('build', r'build\.sh|cmake --build|ninja'), ('tests', r'run_tests\.sh|rulestest|modsettingstest|fwtest|modmenutest|verify'),
                     ('game run', r'rundesk|\.exe\b|wasmrun')):
        if re.search(pat, c):
            return key
    for seg in SEG.split(c):
        seg = seg.strip()
        if not seg or re.match(r'(cd|export|[A-Z_]+=\S*)(\s|$)', seg):
            continue
        prog = os.path.basename(seg.split()[0]).strip('"\'')
        if prog in ('python', 'python3', 'py', 'node', 'bash', 'sh'):
            parts = seg.split()
            arg = next((p for p in parts[1:] if not p.startswith('-')), '')
            return prog + ' ' + (os.path.basename(arg)[:30] if not arg.startswith('<<') else '(inline)')
        return prog[:30]
    return '?'

def tool_kind(name, inp):
    if name == 'Bash' or name == 'PowerShell':
        return name + ': ' + bash_kind(inp.get('command', ''))
    if name == 'Read':
        p = inp.get('file_path', '').replace('\\', '/')
        ext = os.path.splitext(p)[1] or '(none)'
        return 'Read ' + ext
    if name.startswith('mcp__Claude_Browser') or name.startswith('mcp__claude-in-chrome'):
        return 'browser ' + name.split('__')[-1]
    if name.startswith('mcp__'):
        return 'mcp ' + name.split('__')[-1]
    return name

totals = collections.Counter()
by_model = collections.defaultdict(collections.Counter)
by_day = collections.defaultdict(collections.Counter)
carried = collections.Counter()      # category -> token-calls carried
result_tokens = collections.Counter()
result_count = collections.Counter()
per_file = []
big_results = []
reread = collections.Counter()
reread_tokens = collections.Counter()
images = collections.Counter()
send_msgs = [0, 0]
compactions = 0
agents = collections.Counter()
waits = collections.Counter()

GLOBAL_SEEN = set()
SKIP_ATTACH = {'prompt_snapshot', 'thinking_drop', 'credential_org', 'remote_session_change', 'auto_mode', 'deferred_tools_record', 'command_permissions', 'file-history'}
big_writes = [0, 0]
miss_cause = collections.Counter()
expiry_before = collections.Counter()
expiry_class = collections.Counter()
miss_tokens = collections.Counter()
from datetime import datetime
def tparse(x):
    try:
        return datetime.fromisoformat(x.replace('Z', '+00:00'))
    except ValueError:
        return None
writes_ttl = collections.Counter()
by_class = collections.defaultdict(collections.Counter)
first_ctxs = []

def files():
    for root in roots:
        for d, _, names in os.walk(root):
            for n in names:
                if n.endswith('.jsonl'):
                    yield os.path.join(d, n)

for path in files():
    recs = []
    with open(path, encoding='utf-8', errors='replace') as fh:
        for line in fh:
            try:
                r = json.loads(line)
            except ValueError:
                continue
            if isinstance(r, dict):
                recs.append(r)
    title = ''
    # the calls in order, with the pieces of context added before each
    seen_ids = set()
    pieces = []          # (category, tokens) waiting to be charged from the next call on
    live = []            # [category, tokens] carried until a compaction
    calls = 0
    fc = collections.Counter()
    ctx_max = 0
    ctx_sum = 0
    first_ctx = None
    tool_of = {}
    reads_here = collections.Counter()
    in_window_any = False
    prev_ts = None
    last_kind = '(none)'
    after_compact = False
    for r in recs:
        t = r.get('type')
        if t == 'custom-title':
            title = r.get('customTitle') or r.get('title') or title
        ts = r.get('timestamp') or ''
        inwin = since <= ts < until if ts else False
        if t == 'system' and r.get('subtype') == 'compact_boundary':
            live = []
            first_ctx = None
            after_compact = True
            if inwin:
                compactions += 1
                fc['compactions'] += 1
            continue
        m = r.get('message')
        if not isinstance(m, dict):
            if t == 'attachment':
                a = r.get('attachment') or {}
                at = a.get('type', '?') if isinstance(a, dict) else '?'
                if at not in SKIP_ATTACH:
                    pieces.append(('attach: ' + at, tok(len(json.dumps(a)))))
            continue
        content = m.get('content')
        if t == 'assistant':
            if isinstance(content, list):
                for b in content:
                    if not isinstance(b, dict):
                        continue
                    bt = b.get('type')
                    if bt == 'tool_use':
                        name, inp = b.get('name', '?'), b.get('input') or {}
                        kind = tool_kind(name, inp)
                        tool_of[b.get('id')] = (kind, name, inp)
                        last_kind = kind
                        pieces.append(('model: tool inputs', tok(len(json.dumps(inp)))))
                        if inwin:
                            if name in ('Agent', 'Task', 'Workflow'):
                                agents[name] += 1
                            if name in ('SendMessage',) or name.endswith('send_message'):
                                send_msgs[0] += 1
                                send_msgs[1] += len(json.dumps(inp))
                            if name in ('Monitor', 'ScheduleWakeup', 'TaskOutput') or (name == 'Bash' and re.search(r'\bsleep\b|until .*; do', inp.get('command', ''))):
                                waits[name] += 1
                    elif bt == 'text':
                        pieces.append(('model: text', tok(len(b.get('text', '')))))
                    elif bt == 'thinking':
                        pass  # thinking is not carried into later calls
            u = m.get('usage')
            mid = m.get('id') or r.get('requestId') or r.get('uuid')
            if isinstance(u, dict) and mid not in seen_ids:
                seen_ids.add(mid)
                dup = mid in GLOBAL_SEEN
                GLOBAL_SEEN.add(mid)
                # charge the pieces that arrived before this call
                live.extend([list(p) for p in pieces])
                pieces = []
                ctx = int(u.get('input_tokens') or 0) + int(u.get('cache_creation_input_tokens') or 0) + int(u.get('cache_read_input_tokens') or 0)
                if first_ctx is None:
                    first_ctx = ctx - sum(p[1] for p in live)
                    if first_ctx < 0:
                        first_ctx = 0
                if inwin and not dup:
                    in_window_any = True
                    calls += 1
                    ctx_sum += ctx
                    ctx_max = max(ctx_max, ctx)
                    model = m.get('model', '?')
                    for k in ('input_tokens', 'cache_creation_input_tokens', 'cache_read_input_tokens', 'output_tokens'):
                        v = int(u.get(k) or 0)
                        totals[k] += v
                        by_model[model][k] += v
                        by_day[ts[:10]][k] += v
                        fc[k] += v
                    by_model[model]['calls'] += 1
                    by_day[ts[:10]]['calls'] += 1
                    carried['fixed prompt (system, tools, CLAUDE.md, summary)'] += first_ctx
                    est = first_ctx
                    for cat, n in live:
                        carried[cat] += n
                        est += n
                    carried['unattributed (thinking blocks, estimate error)'] += max(0, ctx - est)
                    cw = int(u.get('cache_creation_input_tokens') or 0)
                    cc = u.get('cache_creation') or {}
                    writes_ttl['1h'] += int(cc.get('ephemeral_1h_input_tokens') or 0)
                    writes_ttl['5m'] += int(cc.get('ephemeral_5m_input_tokens') or 0)
                    if cw > 60000:
                        big_writes[0] += 1
                        big_writes[1] += cw
                        gap = (tparse(ts) - tparse(prev_ts)).total_seconds() if prev_ts and tparse(ts) and tparse(prev_ts) else None
                        is1h = int(cc.get('ephemeral_1h_input_tokens') or 0) > int(cc.get('ephemeral_5m_input_tokens') or 0)
                        if calls == 1:
                            cause = 'first call of a session or agent'
                        elif after_compact:
                            cause = 'after a compaction'
                        elif gap is not None and gap > (3600 if is1h else 300):
                            cause = 'gap longer than the cache lifetime (%s)' % ('1h' if is1h else '5m')
                        else:
                            cause = 'other (prefix changed, e.g. model/tools/settings)'
                        miss_cause[cause] += 1
                        if cause.startswith('gap'):
                            expiry_before[last_kind] += 1
                            expiry_class['workflow agents' if 'workflows' in path else ('subagents' if 'subagents' in path else 'main sessions')] += 1
                        miss_tokens[cause] += cw
                    cls = 'workflow agents' if 'workflows' in path else ('subagents' if 'subagents' in path else 'main sessions')
                    by_class[cls]['calls'] += 1
                    by_class[cls]['reads'] += int(u.get('cache_read_input_tokens') or 0)
                    by_class[cls]['writes'] += cw
                    by_class[cls]['out'] += int(u.get('output_tokens') or 0)
                    if calls == 1:
                        first_ctxs.append(ctx)
                    after_compact = False
                prev_ts = ts or prev_ts
            continue
        if t == 'user':
            if isinstance(content, str):
                pieces.append(('user text / injected messages', tok(len(content))))
                continue
            if not isinstance(content, list):
                continue
            for b in content:
                if not isinstance(b, dict):
                    continue
                if b.get('type') == 'tool_result':
                    txt, imgs = text_of(b.get('content'))
                    kind, name, inp = tool_of.get(b.get('tool_use_id'), ('?', '?', {}))
                    n = tok(len(txt)) + imgs * IMAGE_TOKENS
                    pieces.append(('result: ' + kind, n))
                    if inwin:
                        result_tokens[kind] += n
                        result_count[kind] += 1
                        images[kind] += imgs
                        big_results.append((n, kind, os.path.basename(path)[:8], (inp.get('command') or inp.get('file_path') or inp.get('description') or '')[:90]))
                        if name == 'Read':
                            key = inp.get('file_path', '') + '|' + str(inp.get('offset', '')) + '|' + str(inp.get('limit', ''))
                            reads_here[key] += 1
                            if reads_here[key] > 1:
                                reread[os.path.basename(inp.get('file_path', ''))] += 1
                                reread_tokens[os.path.basename(inp.get('file_path', ''))] += n
                elif b.get('type') == 'text':
                    pieces.append(('user text / injected messages', tok(len(b.get('text', '')))))
    if in_window_any:
        rel = os.path.relpath(path, os.path.commonpath(roots) if len(roots) > 1 else roots[0])
        per_file.append((fc, calls, ctx_sum // max(calls, 1), ctx_max, title, rel))

def M(n):
    return '%.1fM' % (n / 1e6) if n < 1e9 else '%.2fB' % (n / 1e9)

print('window %s .. %s' % (since, until))
print('TOTAL calls %d; input %s, cache writes %s, cache reads %s, output %s; compactions %d' % (
    sum(c['calls'] for c in by_model.values()), M(totals['input_tokens']), M(totals['cache_creation_input_tokens']),
    M(totals['cache_read_input_tokens']), M(totals['output_tokens']), compactions))
print('\nBY MODEL')
for model, c in sorted(by_model.items(), key=lambda kv: -kv[1]['cache_read_input_tokens']):
    print('  %-28s calls %6d  reads %8s  writes %7s  out %6s  avg ctx %dk' % (model, c['calls'], M(c['cache_read_input_tokens']),
          M(c['cache_creation_input_tokens']), M(c['output_tokens']),
          (c['cache_read_input_tokens'] + c['cache_creation_input_tokens'] + c['input_tokens']) // max(c['calls'], 1) // 1000))
print('\nBY DAY')
for d in sorted(by_day):
    c = by_day[d]
    print('  %s calls %6d  reads %8s  writes %7s  out %6s' % (d, c['calls'], M(c['cache_read_input_tokens']), M(c['cache_creation_input_tokens']), M(c['output_tokens'])))
print('\nTRANSCRIPTS (top 25 by cache reads): calls, avg ctx, max ctx, compactions, reads, writes, out, title, file')
for fc, calls, avg, mx, title, rel in sorted(per_file, key=lambda x: -x[0]['cache_read_input_tokens'])[:25]:
    print('  %5d %4dk %4dk %2d %8s %7s %6s  %-34s %s' % (calls, avg // 1000, mx // 1000, fc['compactions'], M(fc['cache_read_input_tokens']),
          M(fc['cache_creation_input_tokens']), M(fc['output_tokens']), (title or '')[:34], rel[-70:]))
print('  (%d transcripts in the window)' % len(per_file))
tc = sum(carried.values())
print('\nCONTEXT CARRIED (estimated token-calls, %s total; compare cache reads + writes)' % M(tc))
for cat, n in carried.most_common(40):
    print('  %5.1f%% %9s  %s' % (100.0 * n / tc, M(n), cat))
print('\nTOOL RESULTS (count, tokens, images) by kind, top 30 by tokens')
for k, n in result_tokens.most_common(30):
    print('  %6d %8s %5d  %s' % (result_count[k], M(n), images[k], k))
print('\nLARGEST SINGLE RESULTS')
for n, kind, f, what in sorted(big_results, reverse=True)[:20]:
    print('  %6dk  %-28s %s  %s' % (n // 1000, kind[:28], f, what.replace('\n', ' ')))
print('\nRE-READS of the same file and range within one transcript (top 15 by tokens)')
for k, n in reread_tokens.most_common(15):
    print('  %4d re-reads %8s  %s' % (reread[k], M(n), k))
print('\nAGENTS/WORKFLOWS started %s; messages sent %d (%d chars); waits %s' % (dict(agents), send_msgs[0], send_msgs[1], dict(waits)))

print('\nWRITES by TTL %s; full rewrites (>60k in one call): %d calls, %s' % (dict(writes_ttl), big_writes[0], M(big_writes[1])))
print('first call context of each transcript: median %dk' % (sorted(first_ctxs)[len(first_ctxs)//2] // 1000 if first_ctxs else 0))
print('\nBY CLASS (calls, reads, writes, out)')
for k, c in by_class.items():
    print('  %-16s %6d %8s %7s %6s' % (k, c['calls'], M(c['reads']), M(c['writes']), M(c['out'])))
r, w, o, i = totals['cache_read_input_tokens'], totals['cache_creation_input_tokens'], totals['output_tokens'], totals['input_tokens']
w1, w5 = writes_ttl['1h'], writes_ttl['5m']
if w1 + w5 == 0: w1 = w
cost = {'cache reads': r * 0.20, 'cache writes': w1 * 8.0 + w5 * 5.0, 'output (incl. thinking)': o * 20.0, 'uncached input': i * 4.0}
tot = sum(cost.values())
print('\nAPI-PRICE WEIGHTING (Opus 5.5 per MTok: read $0.20, write 5m $5 / 1h $8, input $4, output $20)')
for k, v in cost.items():
    print('  %-24s $%8.0f  %4.1f%%' % (k, v / 1e6, 100 * v / tot))

print('\nLARGE WRITES BY CAUSE')
for k, n in miss_tokens.most_common():
    print('  %4d calls %7s  %s' % (miss_cause[k], M(n), k))

print('\nEXPIRED-CACHE CALLS: the last tool before the gap, and where')
for k, n in expiry_before.most_common(12):
    print('  %4d  %s' % (n, k))
print('  ' + ', '.join('%s %d' % kv for kv in expiry_class.most_common()))
