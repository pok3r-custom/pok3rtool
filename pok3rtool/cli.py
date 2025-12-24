
import sys
import logging

import typer

from . import cykb, pok3r

log = logging.getLogger(__name__)

app = typer.Typer(
    help="Vortex keyboard firmware update tool",
    pretty_exceptions_enable=False
)


@app.callback()
def app_callback(verbose: bool = False):
    logging.basicConfig(
        level=logging.DEBUG if verbose else logging.INFO,
        stream=sys.stdout,
        datefmt="%H:%M:%S.%f",
        format="%(name)-20s %(levelname)7s %(message)s" if verbose else "%(message)s",
    )
    if not verbose:
        app.pretty_exceptions_enable = True


@app.command()
def list():
    """
    List all known devices
    """
    for device in pok3r.get_devices():
        log.info(device)

    for device in cykb.get_devices():
        log.info(device)
        device.read_info()


if __name__ == "__main__":
    app()
