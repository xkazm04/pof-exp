"""Smoke test module for the /pof/python/run bridge route.

The bridge dispatch wrapper calls this as `pof_bridge_echo.echo(args)` and
expects a dict back. Used by automation tests + manual curl smoke.
"""


def echo(args):
    """Round-trip the provided args. Used to verify the bridge dispatch shape."""
    return {"received": args, "v": args.get("v")}
