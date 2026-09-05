# CardShield Edge

## Windows Runtime Launch

The project uses the MSYS2 UCRT64 runtime. Run the existing binaries through
the project-local helper so that `C:\msys64\ucrt64\bin` is first in `PATH`:

```powershell
.\scripts\run.ps1 engine
.\scripts\run.ps1 simulator --mode mixed --duration 30
```

The helper changes `PATH` only for the launched process. It does not modify
the global Windows environment or copy DLLs into the project.