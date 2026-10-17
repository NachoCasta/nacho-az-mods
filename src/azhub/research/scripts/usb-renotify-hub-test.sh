#!/bin/sh
# usb-renotify.sh - re-announce already-mounted USB media to EP147 after an
# EP147 (re)start. See patch 37-usb-renotify.sh for why.
#
#   usb-renotify.sh              announce, once, every mounted stick EP147 has
#                                not taken (the firmware loader's post-start
#                                path calls it this way)
#   usb-renotify.sh --until-ack  announce, then poll until EP147 has taken every
#                                stick, re-announcing with backoff (1, 2, 4, 4 ... s)
#                                for up to AZ_RENOTIFY_MAX seconds (default 90)
#
# Taken = EP147 holds a file open under the stick's mount point (outside MODS/,
# where the preloaded mods poll). EP147 opens PIONEER/rekordbox/exportLibrary.db
# when it accepts the mount message and keeps it open; while the stick is not
# announced it holds nothing there and BROWSE has no source.
#
# Every attempt is logged to the journal (tag usb-renotify) and to $LOG.
LOG=${AZ_RENOTIFY_LOG:-/tmp/usb-renotify.log}
MAX=${AZ_RENOTIFY_MAX:-90}

up() { cut -d' ' -f1 /proc/uptime; }
T0=$(up)
log() {
    msg="+$(awk -v a="$T0" -v b="$(up)" 'BEGIN{printf "%.1f", b-a}')s $*"
    echo "[$(date '+%H:%M:%S')] $msg" >> "$LOG" 2>/dev/null
    logger -t usb-renotify "$msg" 2>/dev/null
    return 0
}
ep147() { pidof EP147 2>/dev/null | awk '{print $1}'; }
fds() { ls -l /proc/"$1"/fd 2>/dev/null | sed -n 's/.* -> //p'; }

# taken PID MOUNTPOINT: print the file EP147 holds under the mount point
taken() {
    f=$(fds "$1" | grep -F "$2/" | grep -vF "$2/MODS/" | head -1)
    [ -n "$f" ] && echo "$f"
}

# moved_on PID BOX: EP147's storage thread reads the mailboxes in turn (usb1,
# then usb2, ...) and keeps only the one it is waiting on open. If it has
# another one open and not BOX, it has read BOX since it opened it. On the
# emulator its read blocks until a message arrives, so a later message to BOX
# is not read until another mailbox gets one: re-announcing cannot reach it.
moved_on() {
    open=$(fds "$1" | grep -E '^/proc/udev_usb[12]$')
    [ -n "$open" ] && ! echo "$open" | grep -qx "$2"
}

# box_of DEV: the /proc/udev_usbN mailbox for that stick's port
box_of() {
    busid=$(udevadm info --query=path --name="$1" 2>/dev/null | grep -oE '/[12]-1/' | head -1 | tr -d /)
    case "$busid" in
        1-1) echo /proc/udev_usb1 ;;
        2-1) echo /proc/udev_usb2 ;;
        *)   ls /proc/udev_usb1 /proc/udev_usb2 2>/dev/null | head -1 ;;
    esac
}

# pass: one look at every mounted stick; announce those not taken. Sets LEFT
# to how many are still not taken.
SENT=" "
pass() {
    P=$(ep147); LEFT=0
    for mp in /media/usb/sd*; do
        [ -d "$mp" ] || continue
        # Test-only: hub mounts owned by the independent-library prototype.
        topology=$(readlink -f "/sys/class/block/${mp##*/}")
        if printf '%s\n' "$topology" | tr / '\n' | grep -Eq '^[0-9]+-[0-9]+(\.[0-9]+)+$'; then
            continue
        fi
        if [ -f /tmp/azhub-reserved-mounts ] && grep -Fqx "$mp" /tmp/azhub-reserved-mounts; then
            continue
        fi
        line=$(mount | grep " $mp ")
        [ -n "$line" ] || continue
        dev=$(echo "$line" | awk '{print $1}')
        if [ -n "$P" ] && f=$(taken "$P" "$mp"); then
            log "attempt $ATTEMPT: $mp taken by EP147 $P ($f)"
            continue
        fi
        LEFT=$((LEFT + 1))
        if [ -z "$P" ]; then
            log "attempt $ATTEMPT: $mp not announced, EP147 is not running"
            continue
        fi
        BOX=$(box_of "$dev")
        if [ -z "$BOX" ] || [ ! -e "$BOX" ]; then
            log "attempt $ATTEMPT: $mp ($dev) has no mailbox, not announcing"
            continue
        fi
        case "$SENT" in *" $mp "*)
            if moved_on "$P" "$BOX"; then
                log "attempt $ATTEMPT: EP147 $P read $BOX but holds nothing under $mp yet; not re-announcing (it is waiting on another mailbox)"
                continue
            fi ;;
        esac
        fs=$(echo "$line" | sed -E 's/.*type ([^ ]+).*/\1/')
        base=$(echo "$dev" | sed -E 's#/dev/(sd[a-h]).*#\1#')
        ro=$(cat /sys/block/$base/ro 2>/dev/null || echo 0)
        if fds "$P" | grep -qx "$BOX"; then st="EP147 $P has it open"; else st="EP147 $P has not opened it yet, queued"; fi
        echo -n "mount $mp $fs protect:$ro" > "$BOX"
        SENT="$SENT$mp "
        log "attempt $ATTEMPT: announced $mp ($dev, $fs, protect:$ro) -> $BOX ($st)"
    done
}

ATTEMPT=1
if [ "${1:-}" != --until-ack ]; then
    log "one-shot announce"
    pass
    exit 0
fi

log "start: announcing until EP147 has taken every mounted stick (max ${MAX}s)"
delay=1
while :; do
    pass
    if [ "$LEFT" -eq 0 ]; then
        log "done: every mounted stick is taken (attempt $ATTEMPT)"
        exit 0
    fi
    if awk -v a="$T0" -v b="$(up)" -v m="$MAX" 'BEGIN{exit !(b-a>=m)}'; then
        log "GAVE UP: $LEFT stick(s) still not taken after ${MAX}s"
        exit 1
    fi
    # backoff: a slow EP147 gets time to open the library before the next look
    sleep "$delay"
    ATTEMPT=$((ATTEMPT + 1))
    delay=$((delay * 2)); [ "$delay" -gt 4 ] && delay=4
done
