// Copyright PoF. All Rights Reserved.

#include "PofPythonRunner.h"
#include "PillarsOfFortuneBridge.h"
#include "Misc/Paths.h"

#if WITH_EDITOR
#include "IPythonScriptPlugin.h"
#include "PythonScriptTypes.h"
#endif

namespace
{
    constexpr const TCHAR* RESULT_MARKER = TEXT("__POF_BRIDGE_RESULT__");

    /** Escape `'''` so we can safely embed an arbitrary JSON args blob inside a triple-quoted
     *  Python literal. The chance of a literal `'''` inside our JSON is zero (JSON has no triple-quote),
     *  but we belt-and-braces anyway. */
    FString EscapeForTripleQuote(const FString& In)
    {
        return In.Replace(TEXT("'''"), TEXT("\\'\\'\\'"));
    }
}

FPofPythonOutcome UPofPythonRunner::Run(const FString& Module, const FString& Function, const FString& ArgsJson)
{
    FPofPythonOutcome Outcome;

#if WITH_EDITOR
    IPythonScriptPlugin* Py = IPythonScriptPlugin::Get();
    if (!Py || !Py->IsPythonAvailable())
    {
        Outcome.ErrorMessage = TEXT("Python plugin unavailable");
        return Outcome;
    }

    const FString SafeArgs = ArgsJson.IsEmpty() ? TEXT("{}") : EscapeForTripleQuote(ArgsJson);

    // Wrapper:
    //  - capture stdout+stderr in StringIO so log lines flow back to the HTTP layer
    //  - on success: print marker + json({ok:true, data: <fn return>, logs: [...]})
    //  - on exception: print marker + json({ok:false, error: <traceback>, logs: [...]})
    const FString Wrapper = FString::Printf(TEXT(
        "import json, traceback, io, contextlib, importlib\n"
        "_out, _err = io.StringIO(), io.StringIO()\n"
        "with contextlib.redirect_stdout(_out), contextlib.redirect_stderr(_err):\n"
        "    try:\n"
        "        m = importlib.import_module('%s')\n"
        "        try: importlib.reload(m)\n"
        "        except Exception: pass\n"
        "        fn = getattr(m, '%s')\n"
        "        _data = fn(json.loads(r'''%s'''))\n"
        "        _result = {'ok': True, 'data': _data}\n"
        "    except Exception:\n"
        "        _result = {'ok': False, 'error': traceback.format_exc()}\n"
        "_result['logs'] = (_out.getvalue() + _err.getvalue()).splitlines()\n"
        "print('%s' + json.dumps(_result))\n"),
        *Module, *Function, *SafeArgs, RESULT_MARKER);

    FPythonCommandEx Cmd;
    Cmd.Command = Wrapper;
    Cmd.ExecutionMode = EPythonCommandExecutionMode::ExecuteFile;
    // Capture print() output: the Python plugin routes stdout into Cmd.LogOutput
    // (one entry per log line), NOT into Cmd.CommandResult (which only holds the
    // repr of the last evaluated expression in EvaluateStatement mode).
    Py->ExecPythonCommandEx(Cmd);

    // Reassemble the captured stdout/stderr lines and find our marker line.
    FString Captured;
    for (const FPythonLogOutputEntry& Entry : Cmd.LogOutput)
    {
        Captured += Entry.Output;
        Captured += TEXT("\n");
    }
    // Fall back to CommandResult in case a future plugin version populates it.
    if (Captured.IsEmpty())
    {
        Captured = Cmd.CommandResult;
    }

    const int32 Idx = Captured.Find(RESULT_MARKER);
    if (Idx == INDEX_NONE)
    {
        Outcome.ErrorMessage = FString::Printf(TEXT("Python produced no result marker. Raw output: %s"),
            Captured.IsEmpty() ? TEXT("<empty>") : *Captured.Left(2000));
        return Outcome;
    }

    Outcome.ResultJson = Captured.Mid(Idx + FCString::Strlen(RESULT_MARKER)).TrimStartAndEnd();
    // The marker line is followed by a newline; keep only up to it.
    int32 NewlineIdx;
    if (Outcome.ResultJson.FindChar(TEXT('\n'), NewlineIdx))
    {
        Outcome.ResultJson = Outcome.ResultJson.Left(NewlineIdx).TrimStartAndEnd();
    }
    Outcome.bOk = Outcome.ResultJson.Contains(TEXT("\"ok\": true")) ||
                  Outcome.ResultJson.Contains(TEXT("\"ok\":true"));
    return Outcome;
#else
    Outcome.ErrorMessage = TEXT("Python runner is editor-only (compiled without WITH_EDITOR)");
    return Outcome;
#endif
}
