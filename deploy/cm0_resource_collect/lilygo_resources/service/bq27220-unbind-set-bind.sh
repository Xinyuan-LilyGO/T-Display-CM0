#!/bin/sh
set -u

I2C_BUS_NUM="${I2C_BUS_NUM:-1}"
I2C_ADDR="${I2C_ADDR:-0055}"
I2C_DEV="${I2C_DEV:-/dev/i2c-${I2C_BUS_NUM}}"
CAPACITY_MAH="${CAPACITY_MAH:-10000}"
PROGRAM="${PROGRAM:-/usr/local/sbin/batt_capacity_setting}"

DEVICE_ID="${I2C_BUS_NUM}-${I2C_ADDR}"
DEVICE_PATH="/sys/bus/i2c/devices/${DEVICE_ID}"
DRIVER_LINK="${DEVICE_PATH}/driver"
DRIVER_PATH=""
DRIVER_NAME=""
WAS_BOUND=0

log()
{
    echo "[bq27220-capacity] $*"
}

die()
{
    echo "[bq27220-capacity] error: $*" >&2
    exit 1
}

bind_back()
{
    if [ "${WAS_BOUND}" -eq 1 ] && [ -n "${DRIVER_PATH}" ]; then
        if [ -e "${DRIVER_LINK}" ]; then
            log "${DEVICE_ID} already bound, skip bind"
            return
        fi

        if [ -w "${DRIVER_PATH}/bind" ]; then
            log "bind ${DEVICE_ID} back to ${DRIVER_NAME}"
            echo "${DEVICE_ID}" > "${DRIVER_PATH}/bind" || true
        else
            log "cannot write ${DRIVER_PATH}/bind, please bind manually"
        fi
    fi
}

trap bind_back EXIT INT TERM

if [ "$(id -u)" -ne 0 ]; then
    die "please run as root, for example: sudo $0"
fi

[ -e "${I2C_DEV}" ] || die "I2C device not found: ${I2C_DEV}"
[ -x "${PROGRAM}" ] || die "program not executable: ${PROGRAM}"

log "device=${DEVICE_ID}, i2c=${I2C_DEV}, capacity=${CAPACITY_MAH}mAh"

if [ -e "${DRIVER_LINK}" ]; then
    DRIVER_PATH="$(readlink -f "${DRIVER_LINK}")"
    DRIVER_NAME="$(basename "${DRIVER_PATH}")"
    WAS_BOUND=1

    log "${DEVICE_ID} is bound to ${DRIVER_NAME}, unbind first"
    echo "${DEVICE_ID}" > "${DRIVER_PATH}/unbind" || die "unbind ${DEVICE_ID} failed"
    sleep 1
else
    log "${DEVICE_ID} is not bound to a kernel driver"
fi

log "run capacity setting program"
"${PROGRAM}" "${I2C_DEV}" "${CAPACITY_MAH}"
RET=$?

if [ "${RET}" -ne 0 ]; then
    die "capacity setting failed, ret=${RET}"
fi

log "capacity setting done"
exit 0
