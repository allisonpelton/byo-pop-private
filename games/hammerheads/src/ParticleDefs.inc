// ParticleDefs.inc: the particle definitions that the exe builds in code (0x475500-0x4767e0), as rows.
// Written by: python games/hammerheads/tools/particledefs.py --cpp > games/hammerheads/src/ParticleDefs.inc
// Included by Particles.cpp after the row structs EmitterRow and FragmentRow; do not edit by hand.

static const EmitterRow EMITTER_ROWS[] =
{
	{ "GnomeStars", 15, 1500.0, { 0.0, 0.0, 1.0, 0.0, 0.0 }, 3, { 3.0, 3.0, 0.0 }, { 0.0, -1.0, 0.0 }, { 1.0, 1.0, 0.0 }, 400.0, 150.0, { 0.0, 0.0, 0.0 }, 0.0, 0.6, 0.5 },
	{ "GnomeHearts", 15, 100.0, { 1.0, 0.1, 0.1, 0.0, 0.0 }, 3, { 10.0, 10.0, 0.0 }, { 0.0, 0.0, 0.0 }, { 10.0, 10.0, 0.0 }, 400.0, 150.0, { 0.0, 0.0, 0.0 }, 0.0, 1.1, 0.5 },
	{ "GnomeStarsFast", 15, 1500.0, { 0.0, 0.0, 1.0, 0.0, 0.0 }, 3, { 3.0, 3.0, 0.0 }, { 0.0, -1.0, 0.0 }, { 1.0, 1.0, 0.0 }, 800.0, 150.0, { 0.0, 0.0, 0.0 }, 0.0, 0.3, 0.5 },
	{ "GnomeSparkles", 20, 1500.0, { 0.0, 0.0, 1.0, 0.0, 0.0 }, 3, { 3.0, 3.0, 0.0 }, { 0.0, -1.0, 0.0 }, { 1.0, 1.0, 0.0 }, 400.0, 150.0, { 0.0, 0.0, 0.0 }, 0.0, 1.0, 0.5 },
	{ "GnomeSticker", 20, 1500.0, { 0.0, 0.0, 1.0, 0.0, 0.0 }, 3, { 3.0, 3.0, 0.0 }, { 0.0, 0.0, 0.0 }, { 1.0, 1.0, 0.0 }, 400.0, 150.0, { 0.0, 0.0, 0.0 }, 0.0, 0.3, 0.5 },
	{ "GnomeFreeze", 5, 5.0, { 1.0, 1.0, 1.0, 0.1, 0.0 }, 3, { 50.0, 100.0, 0.0 }, { 0.0, 0.5, 0.0 }, { 0.0, 0.1, 0.0 }, 100.0, 50.0, { 0.0, 0.0, 0.0 }, 0.0, 4.5, 0.5 },
	{ "GnomeDust", 20, 1500.0, { 0.0, 0.0, 1.0, 0.0, 0.0 }, 3, { 20.0, 20.0, 0.0 }, { 0.0, -0.5, 0.0 }, { 1.0, 1.0, 0.0 }, 400.0, 150.0, { 0.0, 0.0, 0.0 }, 0.0, 0.1, 0.5 },
	{ "GnomeDustKing", 20, 1500.0, { 0.0, 0.0, 1.0, 0.0, 0.0 }, 3, { 80.0, 80.0, 0.0 }, { 0.0, -0.5, 0.0 }, { 1.0, 1.0, 0.0 }, 400.0, 150.0, { 0.0, 0.0, 0.0 }, 0.0, 0.1, 0.5 },
};

static const FragmentRow FRAGMENT_ROWS[] =
{
	{ "GnomeStarFragment", &IMAGE_STAR_STRIP, 0, true, 0.0, true, 1.0, true, 0.800000011920929, 0, 1.0, { 0.0, 400.0, 0.0 }, 0.0, 0.0, 3.5, 3.5, 0.0 },
	{ "GnomeHeartFragment", &IMAGE_SHOP_HEART, 0, false, 0.0, true, 1.0, true, 0.5, 0, 1.5, { 0.0, 100.0, 0.0 }, 0.0, 0.0, 3.5, 3.5, 0.0 },
	{ "GnomeStarFastFragment", &IMAGE_STAR_STRIP, 0, true, 0.0, true, 1.0, true, 0.30000001192092896, 0, 0.5, { 0.0, 400.0, 0.0 }, 0.0, 0.0, 3.5, 3.5, 0.0 },
	{ "GnomeSparkleFragment", &IMAGE_SPARKLE_STRIP, 1, false, 10.0, true, 1.0, false, 0.5, 1, 1.7999999523162842, { 0.0, 200.0, 0.0 }, 0.0, 0.0, 3.5, 3.5, 0.0 },
	{ "GnomeStickerFragment", &IMAGE_STAR_STRIP, 0, true, 0.0, true, 1.0, true, 0.8999999761581421, 0, 1.7999999523162842, { 0.0, 100.0, 0.0 }, 0.0, 0.0, 3.5, 3.5, 0.0 },
	{ "GnomeFreezeFragment", &IMAGE_SPARKLE_STRIP, 1, false, 10.0, true, 1.0, false, 1.0, 1, 2.0, { 0.0, 100.0, 0.0 }, 0.0, 0.0, 3.5, 3.5, 0.0 },
	{ "GnomeDustFragment", &IMAGE_NAPOLEON_DUST_STRIP, 0, true, 0.0, true, 1.5, true, 0.30000001192092896, 0, 0.6000000238418579, { 0.0, 500.0, 0.0 }, 0.0, 0.0, 3.5, 3.5, 0.0 },
	{ "GnomeDustKingFragment", &IMAGE_KING_DUST_STRIP, 0, true, 0.0, true, 1.5, true, 0.30000001192092896, 0, 0.6000000238418579, { 0.0, 500.0, 0.0 }, 0.0, 0.0, 1.5, 1.5, 0.0 },
};
