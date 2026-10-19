// Pre-script for VS.NET 2002 (MSVC 7.0) binaries: the "Non-Returning Functions - Discovered" analyzer
// wrongly marks the _free / operator delete thunks noreturn and truncates every caller after the call
// (about 2,200 call sites in BookWorm Deluxe). Turn it off before auto-analysis.
// Usage (headless): -preScript DisableNoReturnDiscovery.java   (tools/ghidra_import.sh: MSVC70=1)
//@category Analysis
import ghidra.app.script.GhidraScript;

public class DisableNoReturnDiscovery extends GhidraScript {
	@Override
	public void run() throws Exception {
		setAnalysisOption(currentProgram, "Non-Returning Functions - Discovered", "false");
		println("disabled Non-Returning Functions - Discovered");
	}
}
