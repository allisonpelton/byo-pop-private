// The module loaders libopenmpt's format table (soundlib/Sndfile.cpp, ModuleFormatLoaders) lists but no game uses.
//
// Every music file the games play through libopenmpt is an MO3 (platform/openmpt/OpenMPTPlayer.cpp hands it files that
// start with "MO3"; the IT and S3M tunes go through libxmp-lite). An MO3 is read whole by ReadMO3, which builds the XM, IT,
// S3M, MOD or MTM module itself and calls no other format's loader. The table names every loader, so the linker could drop
// none of them; this file gives 53 of them their symbols as loaders that find nothing, and the build leaves their source
// files out (third_party/CMakeLists.txt, "openmpt-mo3"). Load_it.cpp, Load_dmf.cpp, Load_ams.cpp and Load_s3m.cpp stay: ReadMO3 reaches
// the IT loader's plugin-chunk and extended-property readers and the sample unpackers of DMF and AMS and the S3M
// effect converter, so the IT, DMF, AMS, AMS2 and S3M loaders come with them. libopenmpt's own source is not edited. A game that wants one of
// these formats takes its Load_*.cpp back out of that list and its name out of this file. The web music player, which plays
// every format, builds libopenmpt in full.
#include "stdafx.h"
#include "soundlib/Loaders.h"

OPENMPT_NAMESPACE_BEGIN

CSoundFile::ProbeResult CSoundFile::ProbeFileHeaderXM(MemoryFileReader, const uint64 *) { return ProbeFailure; }
bool CSoundFile::ReadXM(FileReader &, ModLoadingFlags) { return false; }

CSoundFile::ProbeResult CSoundFile::ProbeFileHeaderSTM(MemoryFileReader, const uint64 *) { return ProbeFailure; }
bool CSoundFile::ReadSTM(FileReader &, ModLoadingFlags) { return false; }

CSoundFile::ProbeResult CSoundFile::ProbeFileHeaderMED(MemoryFileReader, const uint64 *) { return ProbeFailure; }
bool CSoundFile::ReadMED(FileReader &, ModLoadingFlags) { return false; }

CSoundFile::ProbeResult CSoundFile::ProbeFileHeaderMTM(MemoryFileReader, const uint64 *) { return ProbeFailure; }
bool CSoundFile::ReadMTM(FileReader &, ModLoadingFlags) { return false; }

CSoundFile::ProbeResult CSoundFile::ProbeFileHeaderMDL(MemoryFileReader, const uint64 *) { return ProbeFailure; }
bool CSoundFile::ReadMDL(FileReader &, ModLoadingFlags) { return false; }

CSoundFile::ProbeResult CSoundFile::ProbeFileHeaderDBM(MemoryFileReader, const uint64 *) { return ProbeFailure; }
bool CSoundFile::ReadDBM(FileReader &, ModLoadingFlags) { return false; }

CSoundFile::ProbeResult CSoundFile::ProbeFileHeaderFAR(MemoryFileReader, const uint64 *) { return ProbeFailure; }
bool CSoundFile::ReadFAR(FileReader &, ModLoadingFlags) { return false; }

CSoundFile::ProbeResult CSoundFile::ProbeFileHeaderOKT(MemoryFileReader, const uint64 *) { return ProbeFailure; }
bool CSoundFile::ReadOKT(FileReader &, ModLoadingFlags) { return false; }

CSoundFile::ProbeResult CSoundFile::ProbeFileHeaderPTM(MemoryFileReader, const uint64 *) { return ProbeFailure; }
bool CSoundFile::ReadPTM(FileReader &, ModLoadingFlags) { return false; }

CSoundFile::ProbeResult CSoundFile::ProbeFileHeaderULT(MemoryFileReader, const uint64 *) { return ProbeFailure; }
bool CSoundFile::ReadULT(FileReader &, ModLoadingFlags) { return false; }

CSoundFile::ProbeResult CSoundFile::ProbeFileHeaderDSM(MemoryFileReader, const uint64 *) { return ProbeFailure; }
bool CSoundFile::ReadDSM(FileReader &, ModLoadingFlags) { return false; }

CSoundFile::ProbeResult CSoundFile::ProbeFileHeaderAMF_Asylum(MemoryFileReader, const uint64 *) { return ProbeFailure; }
bool CSoundFile::ReadAMF_Asylum(FileReader &, ModLoadingFlags) { return false; }

CSoundFile::ProbeResult CSoundFile::ProbeFileHeaderAMF_DSMI(MemoryFileReader, const uint64 *) { return ProbeFailure; }
bool CSoundFile::ReadAMF_DSMI(FileReader &, ModLoadingFlags) { return false; }

CSoundFile::ProbeResult CSoundFile::ProbeFileHeaderPSM(MemoryFileReader, const uint64 *) { return ProbeFailure; }
bool CSoundFile::ReadPSM(FileReader &, ModLoadingFlags) { return false; }

CSoundFile::ProbeResult CSoundFile::ProbeFileHeaderPSM16(MemoryFileReader, const uint64 *) { return ProbeFailure; }
bool CSoundFile::ReadPSM16(FileReader &, ModLoadingFlags) { return false; }

CSoundFile::ProbeResult CSoundFile::ProbeFileHeaderMT2(MemoryFileReader, const uint64 *) { return ProbeFailure; }
bool CSoundFile::ReadMT2(FileReader &, ModLoadingFlags) { return false; }

CSoundFile::ProbeResult CSoundFile::ProbeFileHeaderITP(MemoryFileReader, const uint64 *) { return ProbeFailure; }
bool CSoundFile::ReadITP(FileReader &, ModLoadingFlags) { return false; }

CSoundFile::ProbeResult CSoundFile::ProbeFileHeaderGDM(MemoryFileReader, const uint64 *) { return ProbeFailure; }
bool CSoundFile::ReadGDM(FileReader &, ModLoadingFlags) { return false; }

CSoundFile::ProbeResult CSoundFile::ProbeFileHeaderIMF(MemoryFileReader, const uint64 *) { return ProbeFailure; }
bool CSoundFile::ReadIMF(FileReader &, ModLoadingFlags) { return false; }

CSoundFile::ProbeResult CSoundFile::ProbeFileHeaderDIGI(MemoryFileReader, const uint64 *) { return ProbeFailure; }
bool CSoundFile::ReadDIGI(FileReader &, ModLoadingFlags) { return false; }

CSoundFile::ProbeResult CSoundFile::ProbeFileHeaderDTM(MemoryFileReader, const uint64 *) { return ProbeFailure; }
bool CSoundFile::ReadDTM(FileReader &, ModLoadingFlags) { return false; }

CSoundFile::ProbeResult CSoundFile::ProbeFileHeaderPLM(MemoryFileReader, const uint64 *) { return ProbeFailure; }
bool CSoundFile::ReadPLM(FileReader &, ModLoadingFlags) { return false; }

CSoundFile::ProbeResult CSoundFile::ProbeFileHeaderAM(MemoryFileReader, const uint64 *) { return ProbeFailure; }
bool CSoundFile::ReadAM(FileReader &, ModLoadingFlags) { return false; }

CSoundFile::ProbeResult CSoundFile::ProbeFileHeaderJ2B(MemoryFileReader, const uint64 *) { return ProbeFailure; }
bool CSoundFile::ReadJ2B(FileReader &, ModLoadingFlags) { return false; }

CSoundFile::ProbeResult CSoundFile::ProbeFileHeaderGT2(MemoryFileReader, const uint64 *) { return ProbeFailure; }
bool CSoundFile::ReadGT2(FileReader &, ModLoadingFlags) { return false; }

CSoundFile::ProbeResult CSoundFile::ProbeFileHeaderGTK(MemoryFileReader, const uint64 *) { return ProbeFailure; }
bool CSoundFile::ReadGTK(FileReader &, ModLoadingFlags) { return false; }

CSoundFile::ProbeResult CSoundFile::ProbeFileHeaderPT36(MemoryFileReader, const uint64 *) { return ProbeFailure; }
bool CSoundFile::ReadPT36(FileReader &, ModLoadingFlags) { return false; }

CSoundFile::ProbeResult CSoundFile::ProbeFileHeaderSymMOD(MemoryFileReader, const uint64 *) { return ProbeFailure; }
bool CSoundFile::ReadSymMOD(FileReader &, ModLoadingFlags) { return false; }

CSoundFile::ProbeResult CSoundFile::ProbeFileHeaderMUS_KM(MemoryFileReader, const uint64 *) { return ProbeFailure; }
bool CSoundFile::ReadMUS_KM(FileReader &, ModLoadingFlags) { return false; }

CSoundFile::ProbeResult CSoundFile::ProbeFileHeaderFMT(MemoryFileReader, const uint64 *) { return ProbeFailure; }
bool CSoundFile::ReadFMT(FileReader &, ModLoadingFlags) { return false; }

CSoundFile::ProbeResult CSoundFile::ProbeFileHeaderSFX(MemoryFileReader, const uint64 *) { return ProbeFailure; }
bool CSoundFile::ReadSFX(FileReader &, ModLoadingFlags) { return false; }

CSoundFile::ProbeResult CSoundFile::ProbeFileHeaderSTP(MemoryFileReader, const uint64 *) { return ProbeFailure; }
bool CSoundFile::ReadSTP(FileReader &, ModLoadingFlags) { return false; }

CSoundFile::ProbeResult CSoundFile::ProbeFileHeaderDSym(MemoryFileReader, const uint64 *) { return ProbeFailure; }
bool CSoundFile::ReadDSym(FileReader &, ModLoadingFlags) { return false; }

CSoundFile::ProbeResult CSoundFile::ProbeFileHeaderSTX(MemoryFileReader, const uint64 *) { return ProbeFailure; }
bool CSoundFile::ReadSTX(FileReader &, ModLoadingFlags) { return false; }

CSoundFile::ProbeResult CSoundFile::ProbeFileHeaderUNIC(MemoryFileReader, const uint64 *) { return ProbeFailure; }
bool CSoundFile::ReadUNIC(FileReader &, ModLoadingFlags) { return false; }

CSoundFile::ProbeResult CSoundFile::ProbeFileHeaderMOD(MemoryFileReader, const uint64 *) { return ProbeFailure; }
bool CSoundFile::ReadMOD(FileReader &, ModLoadingFlags) { return false; }

CSoundFile::ProbeResult CSoundFile::ProbeFileHeaderICE(MemoryFileReader, const uint64 *) { return ProbeFailure; }
bool CSoundFile::ReadICE(FileReader &, ModLoadingFlags) { return false; }

CSoundFile::ProbeResult CSoundFile::ProbeFileHeaderKRIS(MemoryFileReader, const uint64 *) { return ProbeFailure; }
bool CSoundFile::ReadKRIS(FileReader &, ModLoadingFlags) { return false; }

CSoundFile::ProbeResult CSoundFile::ProbeFileHeader669(MemoryFileReader, const uint64 *) { return ProbeFailure; }
bool CSoundFile::Read669(FileReader &, ModLoadingFlags) { return false; }

CSoundFile::ProbeResult CSoundFile::ProbeFileHeader667(MemoryFileReader, const uint64 *) { return ProbeFailure; }
bool CSoundFile::Read667(FileReader &, ModLoadingFlags) { return false; }

CSoundFile::ProbeResult CSoundFile::ProbeFileHeaderC67(MemoryFileReader, const uint64 *) { return ProbeFailure; }
bool CSoundFile::ReadC67(FileReader &, ModLoadingFlags) { return false; }

CSoundFile::ProbeResult CSoundFile::ProbeFileHeaderFC(MemoryFileReader, const uint64 *) { return ProbeFailure; }
bool CSoundFile::ReadFC(FileReader &, ModLoadingFlags) { return false; }

CSoundFile::ProbeResult CSoundFile::ProbeFileHeaderFTM(MemoryFileReader, const uint64 *) { return ProbeFailure; }
bool CSoundFile::ReadFTM(FileReader &, ModLoadingFlags) { return false; }

CSoundFile::ProbeResult CSoundFile::ProbeFileHeaderRTM(MemoryFileReader, const uint64 *) { return ProbeFailure; }
bool CSoundFile::ReadRTM(FileReader &, ModLoadingFlags) { return false; }

CSoundFile::ProbeResult CSoundFile::ProbeFileHeaderTCB(MemoryFileReader, const uint64 *) { return ProbeFailure; }
bool CSoundFile::ReadTCB(FileReader &, ModLoadingFlags) { return false; }

CSoundFile::ProbeResult CSoundFile::ProbeFileHeaderCBA(MemoryFileReader, const uint64 *) { return ProbeFailure; }
bool CSoundFile::ReadCBA(FileReader &, ModLoadingFlags) { return false; }

CSoundFile::ProbeResult CSoundFile::ProbeFileHeaderETX(MemoryFileReader, const uint64 *) { return ProbeFailure; }
bool CSoundFile::ReadETX(FileReader &, ModLoadingFlags) { return false; }

CSoundFile::ProbeResult CSoundFile::ProbeFileHeaderDSm(MemoryFileReader, const uint64 *) { return ProbeFailure; }
bool CSoundFile::ReadDSm(FileReader &, ModLoadingFlags) { return false; }

CSoundFile::ProbeResult CSoundFile::ProbeFileHeaderSTK(MemoryFileReader, const uint64 *) { return ProbeFailure; }
bool CSoundFile::ReadSTK(FileReader &, ModLoadingFlags) { return false; }

CSoundFile::ProbeResult CSoundFile::ProbeFileHeaderXMF(MemoryFileReader, const uint64 *) { return ProbeFailure; }
bool CSoundFile::ReadXMF(FileReader &, ModLoadingFlags) { return false; }

CSoundFile::ProbeResult CSoundFile::ProbeFileHeaderPuma(MemoryFileReader, const uint64 *) { return ProbeFailure; }
bool CSoundFile::ReadPuma(FileReader &, ModLoadingFlags) { return false; }

CSoundFile::ProbeResult CSoundFile::ProbeFileHeaderGMC(MemoryFileReader, const uint64 *) { return ProbeFailure; }
bool CSoundFile::ReadGMC(FileReader &, ModLoadingFlags) { return false; }

CSoundFile::ProbeResult CSoundFile::ProbeFileHeaderIMS(MemoryFileReader, const uint64 *) { return ProbeFailure; }
bool CSoundFile::ReadIMS(FileReader &, ModLoadingFlags) { return false; }

OPENMPT_NAMESPACE_END
