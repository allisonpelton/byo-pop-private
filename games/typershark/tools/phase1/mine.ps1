# mine.ps1 [-Kill]: lists (and with -Kill stops) the processes of my Typer Shark phase 1 runs: scripts, origrun, rundesk and the
# games whose command line names this worktree or its tools (never a tool shell: those have shell-snapshots in their command line).
param([switch]$Kill)
$all = Get-CimInstance Win32_Process
foreach ($p in $all) {
    $c = $p.CommandLine
    if (-not $c) { continue }
    if ($c -like '*shell-snapshots*') { continue }
    if ($p.Name -notmatch '^(python|bash|origrun|rundesk|TyperSharkGame|typershark)\.exe$') { continue }
    $mine = ($c -like '*typershark-1c7be7*') -or ($c -like '*tools/phase1/*') -or ($c -like '*tools/typershark/*') -or ($c -like '*phase1\*')
    if (-not $mine) { continue }
    $short = $c.Substring(0, [Math]::Min(110, $c.Length))
    Write-Output ("{0} {1} {2}" -f $p.ProcessId, $p.Name, $short)
    if ($Kill) { Stop-Process -Id $p.ProcessId -Force -ErrorAction SilentlyContinue }
}
