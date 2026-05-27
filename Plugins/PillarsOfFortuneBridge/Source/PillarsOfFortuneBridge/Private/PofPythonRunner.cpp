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
    Py->ExecPythonCommandEx(Cmd);

    // The marker line appears in CommandResult (Python plugin captures stdout).
    const int32 Idx = Cmd.CommandResult.Find(RESULT_MARKER);
    if (Idx == INDEX_NONE)
    {
        Outcome.ErrorMessage = FString::Printf(TEXT("Python produced no result marker. Raw output: %s"),
            Cmd.CommandResult.IsEmpty() ? TEXT("<empty>") : *Cmd.CommandResult);
        return Outcome;
    }

    Outcome.ResultJson = Cmd.CommandResult.Mid(Idx + FCString::Strlen(RESULT_MARKER)).TrimStartAndEnd();
    // Trim trailing newline / extra Python output appended after our marker line
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
