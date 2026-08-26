#!/usr/bin/env bash
set -Eeuo pipefail

SCRIPT_NAME="$(basename "$0")"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
if [ -d "${SCRIPT_DIR}/app" ] && [ -d "${SCRIPT_DIR}/drivers" ] && [ -d "${SCRIPT_DIR}/dtbo" ] && [ -d "${SCRIPT_DIR}/service" ]; then
    SOURCE_DIR="${SCRIPT_DIR}"
else
    SOURCE_DIR="${SCRIPT_DIR}/lilygo_resources"
fi
DEPLOY_DIR="/opt/lilygo_resources"
DESKTOP_APP_DIR="/opt/lilygo_desktop_app"
MUSIC_DIR="/home/pi/Music"
RESOURCE_DIR=""
BOOT_DIR=""
DRY_RUN=0
ENABLE_SERVICES=1
START_SERVICES=0
REPLACE_BOOT_FILES=0
REPLACE_CONFIG_FILE=0
DESKTOP_MODE="none"
LOG_FILE=""

usage() {
    cat <<EOF
Usage: sudo ./${SCRIPT_NAME} [options]

Install LILYGO CM0 Lite resources: apps, kernel modules, device-tree overlays,
boot config entries, and systemd services.

Options:
  --source DIR             Source lilygo_resources directory.
  --deploy-dir DIR         Runtime resource directory. Default: /opt/lilygo_resources
  --desktop-app-dir DIR    Desktop app directory. Default: /opt/lilygo_desktop_app
  --boot-dir DIR           Boot partition directory. Auto-detects /boot/firmware or /boot.
  --replace-config-file    Replace config.txt with bundled file.
                           Default is safer: merge only required entries.
  --replace-boot-files     Replace config.txt and bundled cmdline.txt when present.
                           Default is safer: merge only required entries.
  --mode MODE              Desktop mode: lvgl, raspberry, or none. Default: none.
  --no-enable-services     Install services but do not enable them.
  --start-services         Start services after installation. Reboot is still recommended.
  --dry-run                Print actions without changing the system.
  -h, --help               Show this help.

Examples:
  sudo ./${SCRIPT_NAME}
  sudo ./${SCRIPT_NAME} --mode lvgl
  sudo ./${SCRIPT_NAME} --mode raspberry
  sudo ./${SCRIPT_NAME} --dry-run
  sudo ./${SCRIPT_NAME} --deploy-dir /opt/lilygo_resources
  sudo ./${SCRIPT_NAME} --desktop-app-dir /opt/lilygo_desktop_app
EOF
}

log() {
    printf '[lilygo-install] %s %s\n' "$(date -Is 2>/dev/null || date)" "$*"
}

die() {
    printf '[lilygo-install] error: %s\n' "$*" >&2
    exit 1
}

on_error() {
    local ret=$?
    local line="${1:-unknown}"
    local cmd="${2:-unknown}"

    log "ERROR: command failed"
    log "ERROR: exit_code=${ret}"
    log "ERROR: line=${line}"
    log "ERROR: command=${cmd}"
    log "ERROR: source_dir=${SOURCE_DIR:-unknown}"
    log "ERROR: deploy_dir=${DEPLOY_DIR:-unknown}"
    log "ERROR: desktop_app_dir=${DESKTOP_APP_DIR:-unknown}"
    log "ERROR: boot_dir=${BOOT_DIR:-unknown}"
    log "ERROR: backup_dir=${BACKUP_DIR:-not created}"
    log "ERROR: log_file=${LOG_FILE:-not initialized}"
    return "${ret}"
}

on_exit() {
    local ret=$?

    if [ "${ret}" -eq 0 ]; then
        log "done"
    else
        log "failed: exit_code=${ret}"
        log "check ${LOG_FILE:-the install log} for the last error above"
    fi
}

setup_logging() {
    LOG_FILE="${SOURCE_DIR}/install_lilygo_cm0_lite.log"

    touch "${LOG_FILE}" || die "cannot write install log: ${LOG_FILE}"

    if command -v tee >/dev/null 2>&1; then
        exec > >(tee -a "${LOG_FILE}") 2>&1
    else
        exec >> "${LOG_FILE}" 2>&1
        log "tee not found, logging only to ${LOG_FILE}"
    fi

    trap 'on_error "$LINENO" "$BASH_COMMAND"' ERR
    trap on_exit EXIT

    log "start"
    log "script_name=${SCRIPT_NAME}"
    log "script_dir=${SCRIPT_DIR}"
    log "source_dir=${SOURCE_DIR}"
    log "deploy_dir=${DEPLOY_DIR}"
    log "desktop_app_dir=${DESKTOP_APP_DIR}"
    log "log_file=${LOG_FILE}"
}

run() {
    if [ "${DRY_RUN}" -eq 1 ]; then
        printf '+'
        printf ' %q' "$@"
        printf '\n'
    else
        "$@"
    fi
}

run_may_fail() {
    if [ "${DRY_RUN}" -eq 1 ]; then
        printf '+'
        printf ' %q' "$@"
        printf ' || true\n'
        return
    fi

    if ! "$@"; then
        log "command failed but installation will continue: $*"
    fi
}

escape_sed_replacement() {
    printf '%s' "$1" | sed 's/[&\\#]/\\&/g'
}

write_file() {
    local target="$1"
    local mode="$2"
    local content="$3"

    if [ "${DRY_RUN}" -eq 1 ]; then
        printf '+ write %s mode=%s\n' "${target}" "${mode}"
        printf '%s\n' "${content}"
        return
    fi

    install -d -m 0755 "$(dirname "${target}")"
    printf '%s\n' "${content}" > "${target}"
    chmod "${mode}" "${target}"
}

append_file() {
    local target="$1"
    local content="$2"

    if [ "${DRY_RUN}" -eq 1 ]; then
        printf '+ append %s\n' "${target}"
        printf '%s\n' "${content}"
        return
    fi

    printf '\n%s\n' "${content}" >> "${target}"
}

log_file_content() {
    local label="$1"
    local file="$2"

    if [ ! -f "${file}" ]; then
        log "${label}: ${file} not found"
        return
    fi

    log "${label}: begin ${file}"
    sed 's/^/[lilygo-install]   /' "${file}"
    log "${label}: end ${file}"
}

backup_file() {
    local file="$1"
    [ -e "${file}" ] || return 0

    local rel="${file#/}"
    local dst="${BACKUP_DIR}/${rel}"
    run install -d -m 0755 "$(dirname "${dst}")"
    run cp -a "${file}" "${dst}"
}

realpath_portable() {
    if command -v realpath >/dev/null 2>&1; then
        realpath "$1"
    else
        readlink -f "$1"
    fi
}

parse_args() {
    while [ "$#" -gt 0 ]; do
        case "$1" in
            --source)
                [ "$#" -ge 2 ] || die "--source needs a directory"
                SOURCE_DIR="$2"
                shift 2
                ;;
            --deploy-dir)
                [ "$#" -ge 2 ] || die "--deploy-dir needs a directory"
                DEPLOY_DIR="$2"
                shift 2
                ;;
            --desktop-app-dir)
                [ "$#" -ge 2 ] || die "--desktop-app-dir needs a directory"
                DESKTOP_APP_DIR="$2"
                shift 2
                ;;
            --boot-dir)
                [ "$#" -ge 2 ] || die "--boot-dir needs a directory"
                BOOT_DIR="$2"
                shift 2
                ;;
            --replace-boot-files)
                REPLACE_BOOT_FILES=1
                shift
                ;;
            --replace-config-file)
                REPLACE_CONFIG_FILE=1
                shift
                ;;
            --mode)
                [ "$#" -ge 2 ] || die "--mode needs lvgl, raspberry, or none"
                DESKTOP_MODE="$2"
                case "${DESKTOP_MODE}" in
                    lvgl|raspberry|none) ;;
                    *) die "--mode must be lvgl, raspberry, or none" ;;
                esac
                shift 2
                ;;
            --no-enable-services)
                ENABLE_SERVICES=0
                shift
                ;;
            --start-services)
                START_SERVICES=1
                shift
                ;;
            --dry-run)
                DRY_RUN=1
                shift
                ;;
            -h|--help)
                usage
                exit 0
                ;;
            *)
                die "unknown option: $1"
                ;;
        esac
    done
}

require_root() {
    if [ "$(id -u)" -ne 0 ]; then
        die "please run as root, for example: sudo ./${SCRIPT_NAME}"
    fi
}

require_source_tree() {
    local missing=0
    local item

    for item in \
        "." \
        "app" \
        "drivers" \
        "dtbo" \
        "service" \
        "app/lilygo_desktop_app"
    do
        if [ ! -d "${SOURCE_DIR}/${item}" ]; then
            log "ERROR: missing directory: ${SOURCE_DIR}/${item}"
            missing=1
        fi
    done

    for item in \
        "config.txt" \
        "app/keyboard_lilygo" \
        "app/batt_capacity_setting" \
        "app/lilygo_desktop_app/sample_display.out" \
        "app/lilygo_desktop_app/44k_16.wav" \
        "service/bq27220-unbind-set-bind.sh" \
        "service/batt_capacity_setting.service" \
        "service/keyboard_lilygo.service" \
        "service/lilygo_phone_desktop.service"
    do
        if [ ! -f "${SOURCE_DIR}/${item}" ]; then
            log "ERROR: missing file: ${SOURCE_DIR}/${item}"
            missing=1
        fi
    done

    for item in \
        "drivers/rtc-rx8130.ko" \
        "drivers/qmi8658c.ko" \
        "dtbo/i2c-rtc.dtbo" \
        "dtbo/qmi8658c.dtbo"
    do
        if [ ! -f "${SOURCE_DIR}/${item}" ]; then
            log "ERROR: missing required driver/overlay: ${SOURCE_DIR}/${item}"
            missing=1
        fi
    done

    if [ "${missing}" -ne 0 ]; then
        log "ERROR: source tree is incomplete: ${SOURCE_DIR}"
        for item in app drivers dtbo service; do
            log "listing ${SOURCE_DIR}/${item}:"
            ls -la "${SOURCE_DIR}/${item}" 2>&1 | sed 's/^/[lilygo-install]   /' || true
        done
        return 1
    fi

    log "source tree required files look complete"
}

detect_boot_dir() {
    if [ -n "${BOOT_DIR}" ]; then
        [ -d "${BOOT_DIR}" ] || die "boot directory not found: ${BOOT_DIR}"
        return
    fi

    if [ -d /boot/firmware ]; then
        BOOT_DIR="/boot/firmware"
    elif [ -d /boot ]; then
        BOOT_DIR="/boot"
    else
        die "cannot find boot directory; pass --boot-dir DIR"
    fi
}

install_resource_tree() {
    local src_real
    local dst_real
    local deploy_parent

    log "install resource tree to ${DEPLOY_DIR}"
    deploy_parent="$(dirname "${DEPLOY_DIR}")"
    run install -d -m 0755 "${deploy_parent}"

    src_real="$(realpath_portable "${SOURCE_DIR}")"
    if [ -e "${DEPLOY_DIR}" ]; then
        dst_real="$(realpath_portable "${DEPLOY_DIR}")"
    else
        dst_real=""
    fi

    if [ "${src_real}" = "${dst_real}" ]; then
        log "source and deploy directory are the same, skip resource copy"
    else
        run install -d -m 0755 "${DEPLOY_DIR}"
        run find "${DEPLOY_DIR}" -mindepth 1 -maxdepth 1 -exec rm -rf {} +
        run cp -a "${SOURCE_DIR}/." "${DEPLOY_DIR}/"
    fi

    run chmod 0755 "${DEPLOY_DIR}/app/keyboard_lilygo"
    run chmod 0755 "${DEPLOY_DIR}/app/batt_capacity_setting"

    if [ "${DRY_RUN}" -eq 1 ]; then
        RESOURCE_DIR="${SOURCE_DIR}"
    else
        RESOURCE_DIR="${DEPLOY_DIR}"
    fi
}

install_desktop_app() {
    local src_app_dir
    local src_real
    local dst_real
    local deploy_parent

    src_app_dir="${SOURCE_DIR}/app/lilygo_desktop_app"
    log "install desktop app to ${DESKTOP_APP_DIR}"
    deploy_parent="$(dirname "${DESKTOP_APP_DIR}")"
    run install -d -m 0755 "${deploy_parent}"

    src_real="$(realpath_portable "${src_app_dir}")"
    if [ -e "${DESKTOP_APP_DIR}" ]; then
        dst_real="$(realpath_portable "${DESKTOP_APP_DIR}")"
    else
        dst_real=""
    fi

    if [ "${src_real}" = "${dst_real}" ]; then
        log "source and desktop app directory are the same, skip desktop app copy"
    else
        run install -d -m 0755 "${DESKTOP_APP_DIR}"
        run find "${DESKTOP_APP_DIR}" -mindepth 1 -maxdepth 1 -exec rm -rf {} +
        run cp -a "${src_app_dir}/." "${DESKTOP_APP_DIR}/"
    fi

    run chmod 0755 "${DESKTOP_APP_DIR}/sample_display.out"
}

install_music_file() {
    local source_wav="${RESOURCE_DIR}/app/lilygo_desktop_app/44k_16.wav"
    local target_wav="${MUSIC_DIR}/44k_16.wav"

    [ -f "${source_wav}" ] || die "missing wav file: ${source_wav}"

    log "install 44k_16.wav to ${target_wav}"
    backup_file "${target_wav}"

    if id pi >/dev/null 2>&1; then
        run install -d -m 0755 -o pi -g pi "${MUSIC_DIR}"
        run install -m 0644 -o pi -g pi "${source_wav}" "${target_wav}"
    else
        run install -d -m 0755 "${MUSIC_DIR}"
        run install -m 0644 "${source_wav}" "${target_wav}"
    fi
}

install_user_binaries() {
    log "install user-space binaries"
    run install -d -m 0755 /usr/local/bin /usr/local/sbin
    run install -m 0755 "${RESOURCE_DIR}/app/keyboard_lilygo" /usr/local/bin/keyboard_lilygo
    run install -m 0755 "${RESOURCE_DIR}/app/batt_capacity_setting" /usr/local/sbin/batt_capacity_setting
    run install -m 0755 "${RESOURCE_DIR}/service/bq27220-unbind-set-bind.sh" /usr/local/sbin/bq27220-unbind-set-bind.sh
}

install_runtime_debs() {
    local deb
    local debs=()

    if [ ! -d "${RESOURCE_DIR}/lib" ]; then
        return 0
    fi

    for deb in "${RESOURCE_DIR}"/lib/*.deb; do
        [ -e "${deb}" ] || continue
        debs+=("${deb}")
    done

    [ "${#debs[@]}" -gt 0 ] || return 0

    if ! command -v dpkg >/dev/null 2>&1; then
        log "dpkg not found, skip bundled deb installation"
        return 0
    fi

    log "install runtime deb packages from ${RESOURCE_DIR}/lib"
    if [ "${DRY_RUN}" -eq 1 ]; then
        printf '+ dpkg -i'
        printf ' %q' "${debs[@]}"
        printf '\n'
        return
    fi

    if ! dpkg -i "${debs[@]}"; then
        if command -v apt-get >/dev/null 2>&1; then
            log "fix package dependencies with apt-get -f install"
            if ! apt-get -f install -y; then
                log "bundled runtime debs could not be configured on this image"
                log "continuing without them; install matching Raspberry Pi OS packages or rebuild sample_display.out"
            fi
        else
            log "dpkg -i failed and apt-get is unavailable to fix dependencies"
            log "continuing without bundled runtime debs"
        fi
    fi
}

install_module() {
    local module_name="$1"
    local target_dir="$2"
    local source_file="${RESOURCE_DIR}/drivers/${module_name}"
    local target_file="${target_dir}/${module_name}"

    [ -f "${source_file}" ] || die "missing kernel module: ${source_file}"
    backup_file "${target_file}"
    run install -d -m 0755 "${target_dir}"
    run install -m 0644 "${source_file}" "${target_file}"
}

install_kernel_modules() {
    local kernel_release
    local modules_root
    local module_path
    local module_name
    local target_dir

    kernel_release="$(uname -r)"
    modules_root="/lib/modules/${kernel_release}"

    log "install kernel modules under ${modules_root}"
    for module_path in "${RESOURCE_DIR}"/drivers/*.ko; do
        [ -e "${module_path}" ] || die "no kernel modules found in ${RESOURCE_DIR}/drivers"
        module_name="$(basename "${module_path}")"

        case "${module_name}" in
            goodix_berlin_core.ko|goodix_berlin_i2c.ko)
                target_dir="${modules_root}/kernel/drivers/input/touchscreen"
                ;;
            tca8418_keypad.ko)
                target_dir="${modules_root}/kernel/drivers/input/keyboard"
                ;;
            snd-soc-es8328-i2c.ko|es8389.ko)
                target_dir="${modules_root}/kernel/sound/soc/codecs"
                ;;
            fp5510.ko)
                target_dir="${modules_root}/kernel/drivers/media/i2c"
                ;;
            bq27xxx_battery.ko|bq27xxx_battery_i2c.ko|bq25890_charger.ko)
                target_dir="${modules_root}/kernel/drivers/power/supply"
                ;;
            rtc-rx8130.ko)
                target_dir="${modules_root}/kernel/drivers/rtc"
                ;;
            qmi8658c.ko)
                target_dir="${modules_root}/kernel/drivers/iio/imu"
                ;;
            *)
                target_dir="${modules_root}/extra"
                ;;
        esac

        install_module "${module_name}" "${target_dir}"
    done

    if command -v depmod >/dev/null 2>&1; then
        log "run depmod"
        run depmod -a "${kernel_release}"
    else
        log "depmod not found, skip module dependency refresh"
    fi

}

install_dtbo() {
    local overlays_dir
    local dtbo

    overlays_dir="${BOOT_DIR}/overlays"
    log "install device-tree overlays to ${overlays_dir}"
    run install -d -m 0755 "${overlays_dir}"

    for dtbo in "${RESOURCE_DIR}"/dtbo/*.dtbo; do
        [ -e "${dtbo}" ] || die "no dtbo files found in ${RESOURCE_DIR}/dtbo"
        backup_file "${overlays_dir}/$(basename "${dtbo}")"
        run install -m 0644 "${dtbo}" "${overlays_dir}/$(basename "${dtbo}")"
    done
}

merge_config_txt() {
    local config_file="${BOOT_DIR}/config.txt"
    local marker_begin="# BEGIN LILYGO CM0 Lite"
    local marker_end="# END LILYGO CM0 Lite"
    local block

    block="${marker_begin}
[all]
dtparam=i2c_arm=on
dtparam=i2s=on
dtoverlay=spi0-1cs
dtparam=gpio=on
dtparam=audio=on
enable_uart=1
dtoverlay=disable-bt
dtoverlay=pwm,pin=12,func=0
dtoverlay=batt-charger
dtoverlay=i2c-rtc,rx8130
dtoverlay=i2c_csi_dsi
dtoverlay=imx219
dtoverlay=pwr_ctl
dtoverlay=panel_rm69a10_amoled
dtoverlay=audio_es8388_codec
dtparam=disable_jtag_gpio4=on
dtoverlay=tca8418-keypad
dtoverlay=pwm-gpio,gpio=22
dtoverlay=qmi8658c
framebuffer_depth=32
framebuffer_ignore_alpha=1
${marker_end}"

    if [ "${REPLACE_BOOT_FILES}" -eq 1 ] || [ "${REPLACE_CONFIG_FILE}" -eq 1 ]; then
        log "replace ${config_file} with bundled config.txt"
        backup_file "${config_file}"
        run install -m 0644 "${RESOURCE_DIR}/config.txt" "${config_file}"
        return
    fi

    if [ ! -e "${config_file}" ]; then
        log "${config_file} not found, install bundled config.txt"
        run install -m 0644 "${RESOURCE_DIR}/config.txt" "${config_file}"
        return
    fi

    if grep -Fq "${marker_begin}" "${config_file}"; then
        log "${config_file} already has LILYGO block, skip"
        return
    fi

    log "append LILYGO boot config block to ${config_file}"
    backup_file "${config_file}"
    append_file "${config_file}" "${block}"
}

merge_cmdline_txt() {
    local cmdline_file="${BOOT_DIR}/cmdline.txt"
    local bundled_cmdline="${RESOURCE_DIR}/cmdline.txt"
    local snapshot_file="${BOOT_DIR}/cmdline.txt.before-lilygo-merge"
    local token
    local current
    local runtime_cmdline

    if [ "${REPLACE_BOOT_FILES}" -eq 1 ]; then
        if [ -f "${bundled_cmdline}" ]; then
            if [ -e "${cmdline_file}" ]; then
                run cp -a "${cmdline_file}" "${snapshot_file}"
                log "cmdline snapshot: ${snapshot_file}"
            fi
            log_file_content "cmdline before merge" "${cmdline_file}"
            log "replace ${cmdline_file} with bundled cmdline.txt"
            backup_file "${cmdline_file}"
            run install -m 0644 "${bundled_cmdline}" "${cmdline_file}"
            log_file_content "cmdline after merge" "${cmdline_file}"
            return
        fi

        log "bundled cmdline.txt not found, keep target cmdline.txt and merge required options"
    fi

    if [ ! -e "${cmdline_file}" ]; then
        if [ -f "${bundled_cmdline}" ]; then
            log "${cmdline_file} not found, install bundled cmdline.txt"
            run install -m 0644 "${bundled_cmdline}" "${cmdline_file}"
            log_file_content "cmdline after merge" "${cmdline_file}"
        else
            log "${cmdline_file} not found and no bundled cmdline.txt is available, skip cmdline merge"
        fi
        return
    fi

    run cp -a "${cmdline_file}" "${snapshot_file}"
    log "cmdline snapshot: ${snapshot_file}"
    log_file_content "cmdline before merge" "${cmdline_file}"
    current="$(tr '\n' ' ' < "${cmdline_file}" | sed 's/[[:space:]]\+/ /g; s/^ //; s/ $//')"
    log "cmdline before merge: ${current}"

    if [ -e /proc/cmdline ]; then
        runtime_cmdline="$(tr '\n' ' ' < /proc/cmdline | sed 's/[[:space:]]\+/ /g; s/^ //; s/ $//')"
    else
        runtime_cmdline=""
    fi

    case " ${current} " in
        *" root="*) ;;
        *)
            if [ -n "${runtime_cmdline}" ]; then
                case " ${runtime_cmdline} " in
                    *" root="*)
                        log "WARN: ${cmdline_file} has no root= option, use /proc/cmdline as base"
                        current="${runtime_cmdline}"
                        ;;
                    *)
                        log "WARN: ${cmdline_file} and /proc/cmdline have no root= option"
                        ;;
                esac
            else
                log "WARN: ${cmdline_file} has no root= option and /proc/cmdline is unavailable"
            fi
            ;;
    esac

    for token in vt.global_cursor_default=0 cfg80211.ieee80211_regdom=CN; do
        case " ${current} " in
            *" ${token} "*) ;;
            *) current="${current} ${token}" ;;
        esac
    done

    case " ${current} " in
        *" root="*) ;;
        *)
            log "ERROR: refusing to write ${cmdline_file} without root= option"
            log "ERROR: candidate cmdline: ${current}"
            return 1
            ;;
    esac

    if [ "$(cat "${cmdline_file}")" = "${current}" ]; then
        log "${cmdline_file} already has required cmdline options"
        log_file_content "cmdline after merge" "${cmdline_file}"
        return
    fi

    log "merge LILYGO cmdline options into ${cmdline_file}"
    log "cmdline after merge: ${current}"
    backup_file "${cmdline_file}"
    write_file "${cmdline_file}" 0644 "${current}"
    log_file_content "cmdline after merge" "${cmdline_file}"
}

install_services() {
    local service
    local temp_service

    log "install systemd services"
    run install -d -m 0755 /etc/systemd/system

    for service in batt_capacity_setting.service keyboard_lilygo.service; do
        backup_file "/etc/systemd/system/${service}"
        run install -m 0644 "${RESOURCE_DIR}/service/${service}" "/etc/systemd/system/${service}"
    done

    backup_file /etc/systemd/system/lilygo_phone_desktop.service
    if [ "${DRY_RUN}" -eq 1 ]; then
        printf '+ install generated /etc/systemd/system/lilygo_phone_desktop.service with desktop app dir %s\n' "${DESKTOP_APP_DIR}"
    else
        temp_service="$(mktemp)"
        sed \
            -e "s#__LILYGO_DESKTOP_APP_DIR__#$(escape_sed_replacement "${DESKTOP_APP_DIR}")#g" \
            -e "s#__LILYGO_RESOURCE_DIR__#$(escape_sed_replacement "${RESOURCE_DIR}")#g" \
            "${RESOURCE_DIR}/service/lilygo_phone_desktop.service" > "${temp_service}"
        install -m 0644 "${temp_service}" /etc/systemd/system/lilygo_phone_desktop.service
        rm -f "${temp_service}"
    fi

    if command -v systemctl >/dev/null 2>&1; then
        run systemctl daemon-reload

        if [ "${ENABLE_SERVICES}" -eq 1 ]; then
            run systemctl enable batt_capacity_setting.service

            case "${DESKTOP_MODE}" in
                lvgl)
                    log "configure LVGL desktop mode"
                    run systemctl disable keyboard_lilygo.service
                    run systemctl enable lilygo_phone_desktop.service
                    run systemctl set-default multi-user.target
                    ;;
                raspberry)
                    log "configure Raspberry Pi desktop mode"
                    run systemctl enable keyboard_lilygo.service
                    run systemctl disable lilygo_phone_desktop.service
                    run systemctl set-default graphical.target
                    ;;
                none)
                    log "desktop mode is none, leave keyboard and LVGL services unchanged"
                    ;;
            esac
        fi

        if [ "${START_SERVICES}" -eq 1 ]; then
            run_may_fail systemctl restart batt_capacity_setting.service

            case "${DESKTOP_MODE}" in
                lvgl)
                    run_may_fail systemctl stop keyboard_lilygo.service
                    run_may_fail systemctl restart lilygo_phone_desktop.service
                    ;;
                raspberry)
                    run_may_fail systemctl restart keyboard_lilygo.service
                    run_may_fail systemctl stop lilygo_phone_desktop.service
                    ;;
                none) ;;
            esac
        fi
    else
        log "systemctl not found, skip service enable"
    fi
}

check_runtime_deps() {
    local binary="${DESKTOP_APP_DIR}/sample_display.out"
    local lib_dir="${RESOURCE_DIR}/lib"
    local missing

    [ -x "${binary}" ] || return 0

    if ! command -v ldd >/dev/null 2>&1; then
        log "ldd not found, skip runtime dependency check"
        return 0
    fi

    if [ -d "${lib_dir}" ]; then
        missing="$(LD_LIBRARY_PATH="${lib_dir}${LD_LIBRARY_PATH:+:${LD_LIBRARY_PATH}}" ldd "${binary}" 2>/dev/null | awk '/not found/ {print $1}' | sort -u || true)"
    else
        missing="$(ldd "${binary}" 2>/dev/null | awk '/not found/ {print $1}' | sort -u || true)"
    fi
    if [ -n "${missing}" ]; then
        log "sample_display.out is missing runtime libraries:"
        printf '%s\n' "${missing}" | sed 's/^/[lilygo-install]   /'
        log "install the matching packages or rebuild sample_display.out for this OS image"
    else
        log "sample_display.out runtime dependencies look OK"
    fi
}

main() {
    parse_args "$@"
    [ -d "${SOURCE_DIR}" ] || die "source directory not found: ${SOURCE_DIR}"
    SOURCE_DIR="$(realpath_portable "${SOURCE_DIR}")"
    setup_logging
    require_root
    require_source_tree
    detect_boot_dir
    log "boot_dir=${BOOT_DIR}"
    log "desktop_mode=${DESKTOP_MODE}"
    log "dry_run=${DRY_RUN}"
    log "replace_boot_files=${REPLACE_BOOT_FILES}"
    log "replace_config_file=${REPLACE_CONFIG_FILE}"

    BACKUP_DIR="/var/backups/lilygo-cm0-lite/$(date +%Y%m%d-%H%M%S)"
    log "backup directory: ${BACKUP_DIR}"
    run install -d -m 0755 "${BACKUP_DIR}"

    install_resource_tree
    install_desktop_app
    install_music_file
    install_user_binaries
    install_runtime_debs
    install_kernel_modules
    install_dtbo
    merge_config_txt
    merge_cmdline_txt
    install_services
    check_runtime_deps

    log "installation complete"
    log "please reboot the board to load new overlays and kernel modules"
}

main "$@"
