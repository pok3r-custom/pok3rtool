
import sys
import logging
from pathlib import Path
from typing import Annotated
from enum import Enum

import typer
from rich.console import Console
from rich.logging import RichHandler

from . import cykb, pok3r, package

log = logging.getLogger(__name__)

app = typer.Typer(
    help="Vortex keyboard firmware update tool",
    # the pretty exceptions are ugly in non-terminal output
    pretty_exceptions_enable=False,
)


@app.callback()
def app_callback(
        verbose: int = typer.Option(0, "--verbose", "-v", count=True, help="Verbosity level"),
        rich: bool = True,
):
    if verbose > 1:
        level = logging.NOTSET
    elif verbose > 0:
        level = logging.DEBUG
    else:
        level = logging.INFO

    if not Console().is_terminal:
        # RichHandler has a very poor fallback to a wrapped 80-column mode for non-terminals
        # so just don't use RichHandler if the output is not a terminal
        rich = False

    if rich:
        # turn pretty exceptions back on in a terminal
        app.pretty_exceptions_enable = True
        if verbose > 1:
            handler = RichHandler()
        elif verbose > 0:
            handler = RichHandler(show_time=False, show_path=False)
        else:
            handler = RichHandler(show_time=False, show_level=False, show_path=False)
        handler.setFormatter(logging.Formatter("%(message)s"))
    else:
        if verbose > 1:
            handler = logging.StreamHandler(stream=sys.stdout)
            handler.setFormatter(logging.Formatter("%(asctime)s.%(msecs)03d %(levelname)-7s %(message)s", datefmt="%H:%M:%S"))
        elif verbose > 0:
            handler = logging.StreamHandler(stream=sys.stdout)
            handler.setFormatter(logging.Formatter("%(levelname)-7s %(message)s"))
        else:
            handler = logging.StreamHandler(stream=sys.stdout)
            handler.setFormatter(logging.Formatter("%(message)s"))

    logging.basicConfig(level=level, handlers=[handler])


def find_devices():
    for name, device in pok3r.get_devices():
        yield name, device
    for name, device in cykb.get_devices():
        yield name, device


def find_device():
    devs = list(find_devices())
    if not len(devs):
        log.error("No devices found")
        raise typer.Exit(2)
    elif len(devs) > 1:
        log.error("Too many devices! Disconnect other devices")
        raise typer.Exit(3)

    name, device = devs[0]
    log.info(f"Device: {name}")
    return device


@app.command("list")
def cmd_list():
    """List connected devices"""
    for name, device in find_devices():
        with device:
            log.info(f"{name}: {device.version()}")


@app.command("version")
def cmd_version(version: Annotated[str, typer.Argument()] = None):
    """Read or write device version"""
    device = find_device()
    with device:
        if version:
            device.write_version(version)
        else:
            vstr = device.version()
            log.info(f"Version: {vstr}")


@app.command("reboot")
def cmd_reboot(bootloader: bool = False):
    """Reboot device"""
    device = find_device()
    with device:
        device.reboot(bootloader)


@app.command("flash")
def cmd_flash(version: str, file: Path):
    """Flash device firmware"""
    with open(file, "rb") as f:
        fw_data = f.read()
    device = find_device()
    with device:
        device.flash(version, fw_data, progress=True)


@app.command("leak")
def cmd_leak(output: Path):
    device = find_device()
    if isinstance(device, pok3r.POK3R_Device):
        with device:
            data = device.leak_flash()
        with open(output, "wb") as f:
            f.write(data)
    else:
        log.info("Not supported for this devie")


class UpdateFormat(str, Enum):
    MAAJONSN = "maajonsn"
    MAAV102 = "maav102"
    MAAV105 = "maav105"
    KBP_CYKB = "kbp_cykb"


@app.command("extract")
def cmd_extract(format: Annotated[UpdateFormat, typer.Argument(case_sensitive=False)], file: Path, output: Annotated[Path, typer.Argument()] = None):
    """Extract firmware from update EXE"""
    match format.lower():
        case UpdateFormat.MAAJONSN:
            package.extract_maajonsn(file, output)
        case UpdateFormat.MAAV102:
            package.extract_maav102(file, output)
        case UpdateFormat.MAAV105:
            package.extract_maav105(file, output)
        case UpdateFormat.KBP_CYKB:
            package.extract_kbp_cykb(file, output)
        case _:
            raise typer.BadParameter(f"Unknown format {format}")


if __name__ == "__main__":
    app()
