
import sys
import logging
from pathlib import Path
from typing import Annotated

import typer

from . import cykb, pok3r, package

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
    """List all known devices"""
    for device in pok3r.get_devices():
        log.info(device)

    for device in cykb.get_devices():
        log.info(device)
        device.read_info()


@app.command()
def extract(format: str, file: Path, output: Annotated[Path, typer.Argument()] = None):
    """Extract firmware from update EXE"""
    match format.lower():
        case "maajonsn":
            package.extract_maajonsn(file, output)
        case "maav102":
            package.extract_maav102(file, output)
        case "maav105":
            package.extract_maav105(file, output)
        case _:
            raise typer.BadParameter(f"Unknown format {format}")


if __name__ == "__main__":
    app()
