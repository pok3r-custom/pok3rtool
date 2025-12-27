
import sys
import logging
from pathlib import Path
from typing import Annotated, Generator
from enum import Enum

import typer
from rich.console import Console
from rich.logging import RichHandler

from . import cykb, pok3r, package
from .device import Device

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


def find_devices() -> Generator[tuple[str, Device], None, None]:
    for name, device in pok3r.get_devices():
        yield name, device
    for name, device in cykb.get_devices():
        yield name, device


def find_device(devnum: int | None = None, selectable: bool = True) -> Device:
    devs = list(find_devices())
    if not len(devs):
        log.error("No devices found")
        raise typer.Exit(2)

    if devnum is None:
        if len(devs) > 1:
            if selectable:
                log.error("Multiple devices found. Specify device or disconnect other devices.")
            else:
                log.error("Multiple devices found. Disconnect other devices.")
        else:
            devnum = 0

    if devnum is not None:
        if devnum < len(devs):
            name, device = devs[devnum]
            log.info(f"Device: {name}")
            return device
        else:
            log.error("Invalid device number!")

    if selectable:
        log.info(f"Devices:")
        for i, (name, device) in enumerate(devs):
            log.info(f"{i}: {name}")

    raise typer.Exit(2)


@app.command("list")
def cmd_list():
    """List connected devices"""
    for i, (name, device) in enumerate(find_devices()):
        with device:
            log.info(f"{i}: {name} - {device.version()}")


@app.command("version")
def cmd_version(
        version: Annotated[str, typer.Argument(help="Version to write")] = None,
        devnum: int = typer.Option(None, "--devnum", "-n", help="Device number")
):
    """Read or write device version"""
    device = find_device(devnum)
    with device:
        if version:
            device.write_version(version)
        else:
            vstr = device.version()
            log.info(f"Version: {vstr}")


@app.command("reboot")
def cmd_reboot(
        bootloader: bool = False,
        devnum: int | None = typer.Option(None, "--devnum", "-n", help="Device number")
):
    """Reboot device"""
    device = find_device(devnum)
    with device:
        device.reboot(bootloader)


@app.command("flash")
def cmd_flash(
        version: Annotated[str, typer.Argument(help="Version to write")],
        file: Annotated[Path, typer.Argument(help="Firmware file to flash")],
        reboot: bool = True
):
    """Flash device firmware"""
    with open(file, "rb") as f:
        fw_data = f.read()
    # i don't want to deal with the edge cases around selecting a device and
    # re-finding that device when rebooting it
    device = find_device(selectable=False)
    with device:
        device.flash(version, fw_data, progress=True)


@app.command("dump")
def cmd_dump(
        output: Path,
        devnum: int = typer.Option(None, "--devnum", "-n", help="Device number")
):
    """Dump device flash"""
    device = find_device(devnum)
    with device:
        data = device.dump()
    with open(output, "wb") as f:
        f.write(data)


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
