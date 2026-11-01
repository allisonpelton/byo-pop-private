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
# Checked 2026-10-26: Rocket Mania Deluxe (PopCap, 2004) keeps its Steam page (app 3440), which says it is no longer
# available on the Steam store; EA's library does not list it, Amazon has only an old disc, and GameHouse, iWin, Big Fish
# Games and Pogo showed no page for it (their search pages render with JavaScript, so a listing could have been missed).
set(BYOPOP_BUY_rocketmania 2026-10-26)
# Checked 2026-10-26: Bookworm Adventures Volume 2 (PopCap, 2009; Steam app 3630) has no Steam page any more (it
# redirects to the store front and the store API knows no such app) and is not in Steam's store search; it is not on
# EA's PopCap games page and EA has no game page for it; iWin's search finds none, Pogo has only the original Bookworm,
# and Amazon's download code is unavailable (the rest are old discs). Like Volume 1 it was delisted in 2016. GameHouse
# and Big Fish Games render their search only with JavaScript and EA's library search showed no results, so those three
# were not checked.
set(BYOPOP_BUY_bookwormadv2 2026-10-26)
# Checked 2026-10-26: Dynomite Deluxe (PopCap) is on sale on Steam (app 3380, published by EA). GameHouse, Big Fish
# Games, iWin and Pogo showed no page for it, and Amazon only an unrelated Android app. EA's store sells "Dynomite!" for
# PC and Mac through the EA app (rechecked 2026-11-01). It is the same game: every PC and Mac release of a PopCap game
# is its Deluxe edition, which is only the naming convention (the user, 2026-11-01).
set(BYOPOP_BUY_dynomite 2026-11-01
  Steam "https://store.steampowered.com/app/3380/Dynomite_Deluxe/"
  "EA app" "https://www.ea.com/games/dynomite/dynomite")
# Checked 2026-11-01: Mahjong Escape: Ancient Japan and Mahjong Escape: Ancient China (SpinTop Games, sold by PopCap in
# the PopCap Game Collection) are not in Steam's store search ("mahjong escape": no results) and not on EA's PopCap page;
# iWin's search and GameHouse's mahjong games showed neither in their first page of results (the rest load only with
# JavaScript), Pogo's search renders only with JavaScript, Big Fish Games' search page was not found and Amazon refused
# the automated visit, so those last three were not checked.
set(BYOPOP_BUY_mahjongjapan 2026-11-01)
set(BYOPOP_BUY_mahjongchina 2026-11-01)
# Checked 2026-11-01: Bonnie's Bookstore Deluxe (New Crayon Games, published by PopCap) is not in Steam's store search
# ("bonnie's bookstore": no results) and not on EA's PopCap page; other stores were not checked.
set(BYOPOP_BUY_bonnies 2026-11-01)
# Checked 2026-11-01: Steam sells Bejeweled Twist (app 3560, as in the user's own library) and the EA app sells it (its
# page on ea.com); other stores were not checked.
set(BYOPOP_BUY_bejeweledtwist 2026-11-01
  Steam "https://store.steampowered.com/app/3560/"
  "EA app" "https://www.ea.com/games/bejeweled/bejeweled-twist")
