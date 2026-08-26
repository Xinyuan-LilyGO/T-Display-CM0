#!/bin/bash
set -Ee

USERNAME="pi"
PASSWORD="raspberry"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BOOT_DIR="/boot"
[ -f /boot/firmware/cmdline.txt ] && BOOT_DIR="/boot/firmware"
LOG_FILE="${SCRIPT_DIR}/firstrun-lilygo.log"
INSTALLER="${SCRIPT_DIR}/lilygo_resources/install_lilygo_cm0_lite.sh"
FIRSTBOOT_SERVICE="/etc/systemd/system/lilygo-firstboot-install.service"
RESOURCE_DIR="${SCRIPT_DIR}/lilygo_resources"

log()
{
    echo "[firstrun] $*"
}

on_error()
{
    local ret=$?
    log "ERROR: exit_code=${ret} line=${1:-unknown} command=${2:-unknown}"
    return "${ret}"
}

on_exit()
{
    local ret=$?
    if [ "${ret}" -eq 0 ]; then
        log "done: $(date -Is 2>/dev/null || date)"
    else
        log "failed: exit_code=${ret}"
    fi
}

make_password_hash()
{
    if command -v openssl >/dev/null 2>&1; then
        printf '%s\n' "${PASSWORD}" | openssl passwd -6 -stdin
    elif command -v mkpasswd >/dev/null 2>&1; then
        mkpasswd -m sha-512 "${PASSWORD}"
    else
        return 1
    fi
}

prepare_userconf()
{
    local hash
    hash="$(make_password_hash)" || {
        log "ERROR: openssl or mkpasswd is required to create userconf"
        return 1
    }

    rm -f "${BOOT_DIR}/userconf"
    printf '%s:%s\n' "${USERNAME}" "${hash}" > "${BOOT_DIR}/userconf.txt"
    chmod 0600 "${BOOT_DIR}/userconf.txt" 2>/dev/null || true
    rm -f "${BOOT_DIR}/failed_userconf" "${BOOT_DIR}/failed_userconf.txt"
    log "wrote ${BOOT_DIR}/userconf.txt"

    if command -v systemctl >/dev/null 2>&1; then
        systemctl unmask userconfig.service >/dev/null 2>&1 || true
        systemctl enable userconfig.service >/dev/null 2>&1 || true
        log "userconfig.service left enabled"
    fi
}

clean_cmdline()
{
    local file="${BOOT_DIR}/cmdline.txt"
    local current cleaned="" token runtime_cmdline

    [ -f "${file}" ] || return 0
    log "cmdline byte size before cleanup: $(wc -c < "${file}" 2>/dev/null || echo unknown)"
    if ! LC_ALL=C tr -d '\000' < "${file}" | cmp -s - "${file}"; then
        log "WARN: ${file} contains NUL bytes"
    fi

    current="$(tr -d '\000' < "${file}" | tr '\n' ' ' | sed 's/[[:space:]]\+/ /g; s/^ //; s/ $//')"
    log "cmdline before cleanup: ${current}"

    case " ${current} " in
        *" root="*) ;;
        *)
            if [ -f /proc/cmdline ]; then
                runtime_cmdline="$(tr -d '\000' < /proc/cmdline | tr '\n' ' ' | sed 's/[[:space:]]\+/ /g; s/^ //; s/ $//')"
                log "cmdline fallback from /proc/cmdline: ${runtime_cmdline}"
                case " ${runtime_cmdline} " in
                    *" root="*) current="${runtime_cmdline}" ;;
                esac
            fi
            ;;
    esac

    for token in ${current}; do
        case "${token}" in
            systemd.run=/boot/firstrun.sh|\
            systemd.run=/boot/firmware/firstrun.sh|\
            systemd.run_success_action=*|\
            systemd.run_failure_action=*|\
            systemd.unit=kernel-command-line.target)
                ;;
            *)
                cleaned="${cleaned:+${cleaned} }${token}"
                ;;
        esac
    done

    case " ${cleaned} " in
        *" root="*) ;;
        *)
            log "ERROR: refusing to write ${file}: cleaned cmdline has no root= option"
            log "ERROR: cleaned cmdline candidate: ${cleaned}"
            return 1
            ;;
    esac

    printf '%s\n' "${cleaned}" > "${file}"
    log "cmdline after cleanup: ${cleaned}"
}

verify_installer_resources()
{
    local missing=0
    local item

    [ -f "${INSTALLER}" ] || {
        log "ERROR: installer not found: ${INSTALLER}"
        return 1
    }

    for item in \
        "config.txt" \
        "app/keyboard_lilygo" \
        "app/batt_capacity_setting" \
        "app/lilygo_desktop_app/sample_display.out" \
        "app/lilygo_desktop_app/44k_16.wav" \
        "drivers/rtc-rx8130.ko" \
        "drivers/qmi8658c.ko" \
        "dtbo/i2c-rtc.dtbo" \
        "dtbo/qmi8658c.dtbo" \
        "service/bq27220-unbind-set-bind.sh" \
        "service/batt_capacity_setting.service" \
        "service/keyboard_lilygo.service" \
        "service/lilygo_phone_desktop.service"
    do
        if [ ! -e "${RESOURCE_DIR}/${item}" ]; then
            log "ERROR: missing required resource: ${RESOURCE_DIR}/${item}"
            missing=1
        fi
    done

    if [ "${missing}" -ne 0 ]; then
        log "ERROR: lilygo_resources is incomplete; do not install firstboot service"
        log "service directory listing:"
        ls -la "${RESOURCE_DIR}/service" 2>&1 | sed 's/^/[firstrun]   /' || true
        return 1
    fi

    log "lilygo_resources required files look complete"
}

install_firstboot_service()
{
    cat > "${FIRSTBOOT_SERVICE}" <<EOF
[Unit]
Description=LILYGO first boot installer
After=multi-user.target userconfig.service
Wants=multi-user.target

[Service]
Type=oneshot
ExecStart=/bin/bash ${INSTALLER} --mode lvgl --replace-config-file
ExecStartPost=/bin/systemctl disable lilygo-firstboot-install.service
ExecStartPost=/bin/systemctl reboot
StandardOutput=journal+console
StandardError=journal+console

[Install]
WantedBy=multi-user.target
EOF

    systemctl daemon-reload
    systemctl enable lilygo-firstboot-install.service
    log "enabled lilygo-firstboot-install.service"
}

exec >> "${LOG_FILE}" 2>&1
trap 'on_error "$LINENO" "$BASH_COMMAND"' ERR
trap on_exit EXIT

log "stage1 start: $(date -Is 2>/dev/null || date)"
log "script_dir=${SCRIPT_DIR}"
log "boot_dir=${BOOT_DIR}"

prepare_userconf
log "enable ssh service"
if ! systemctl enable ssh; then
    log "WARN: failed to enable ssh service, continue"
fi

verify_installer_resources
install_firstboot_service
clean_cmdline
sync
log "stage1 reboot"
reboot -f
log "ERROR: reboot returned unexpectedly"
exit 1
