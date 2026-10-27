# Where each game can be bought digitally, shown under its terms on its page and its copyright page
# (docs/WEB_PLAYER.md, "Where to buy"). Every game with a web page needs an entry; byopop_web_shell() stops at one
# without. An entry is the date the stores were checked, then a store's name and its page for the game, for each store
# that sells it:
#   set(BYOPOP_BUY_<target> <YYYY-MM-DD> [<store> <url>]...)
# With no store, the page says the game is not for sale as of that date. Only stores that sell the game new and
# digitally under the publisher's license, linked on their own trusted sites: the publisher's store (the EA app),
# platforms like Steam, and established game stores (Big Fish Games, GameHouse, iWin, Pogo, Amazon's downloads and the
# like). No key resellers and no second-hand sales (old discs on Amazon or eBay).
# Checked 2026-10-25: Steam (its store search and app pages) and EA's store had Bejeweled Deluxe (Steam) and Big Money
# Deluxe (Steam, EA app); BookWorm Deluxe was delisted from Steam, Origin and Pogo in 2016; Big Fish Games, GameHouse,
# iWin, Pogo and Amazon showed none of these games for sale (Amazon only old discs); the Humble Store blocks automated
# visits and was not checked.
set(BYOPOP_BUY_atomica 2026-10-25)
set(BYOPOP_BUY_bejeweled 2026-10-25
  Steam "https://store.steampowered.com/app/3350/Bejeweled_Deluxe/")
set(BYOPOP_BUY_bigmoney 2026-10-25
  Steam "https://store.steampowered.com/app/3360/"
  "EA app" "https://www.ea.com/games/big-money/big-money")
set(BYOPOP_BUY_bookworm 2026-10-25)
set(BYOPOP_BUY_noahsark 2026-10-25)
set(BYOPOP_BUY_sevenseas 2026-10-25)
# Games being ported, checked the same day (Steam's store search, EA's store): none for sale. Bookworm Adventures was
# delisted with BookWorm in 2016.
set(BYOPOP_BUY_bookwormadv 2026-10-25)
set(BYOPOP_BUY_mummymaze 2026-10-25)
set(BYOPOP_BUY_ningpo 2026-10-25)
set(BYOPOP_BUY_pixelus 2026-10-25)
set(BYOPOP_BUY_tiptop 2026-10-25)
# Checked 2026-10-26: the EA app sells Alchemy (its page on ea.com); Steam's store search has no Alchemy of PopCap's.
set(BYOPOP_BUY_alchemy 2026-10-26
  "EA app" "https://www.ea.com/games/alchemy/alchemy")
# Checked 2026-10-26: Word Harmony (Sprout Games, 2006) is not in Steam's store search or on
# EA's store, GameHouse has no page for it (only a support note that it does not suit Windows 10 and 11), and a search
# of Big Fish Games, iWin and Pogo found none.
set(BYOPOP_BUY_wordharmony 2026-10-26)
# Checked 2026-10-26: Cosmic Bugs (Retro64, published by PopCap in 2005) is not in Steam's store search or on EA's
# store, Retro64's site no longer answers, and GameHouse names it only on a support page of games that suit Windows 10
# and 11, with no store page found there or at Big Fish Games, iWin or Pogo.
set(BYOPOP_BUY_cosmicbugs 2026-10-26)
# Checked 2026-10-26: Water Bugs (Retro64, published by PopCap in 2005) is not in Steam's store search or on PopCap's
# Steam publisher page, not in EA's library, and has no store page at GameHouse, Big Fish Games, iWin or Pogo; Retro64's
# site no longer answers, and only third-party archive pages mention it.
set(BYOPOP_BUY_waterbugs 2026-10-26)
# Bookworm Adventures Volume 2 (PopCap, 2009): a placeholder that only lets the web configure pass. Its stores are
# checked at the web milestone, before a page is published.
set(BYOPOP_BUY_bookwormadv2 2026-10-26)
# Dynomite Deluxe (PopCap, 2003): a placeholder that only lets the web configure pass. Its stores are checked by the
# coordinator before a page is published.
set(BYOPOP_BUY_dynomite 2026-10-26)
