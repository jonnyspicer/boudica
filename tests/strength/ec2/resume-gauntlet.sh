#!/usr/bin/env bash
# Restart an unfinished gauntlet after a spot interruption. A persistent spot
# request with interruption behaviour "stop" boots the same disk again when
# capacity returns, but nothing restarts the games; this does, from a systemd
# service. gauntlet.sh writes $CHESS_DIR/results/active while a run is unfinished.
#
# Usage (on the instance):
#   sudo tests/strength/ec2/resume-gauntlet.sh install   # enable at boot
#   tests/strength/ec2/resume-gauntlet.sh                # resume now, if a run is active
set -euo pipefail

CHESS_DIR="${CHESS_DIR:-/opt/chess}"
REPO="$(cd "$(dirname "$0")/../../.." && pwd)"
USER_NAME="${SUDO_USER:-$(id -un)}"

if [[ "${1:-}" == "install" ]]; then
    cat > /etc/systemd/system/resume-gauntlet.service <<EOF
[Unit]
Description=Resume an unfinished Boudica gauntlet after a restart
After=network-online.target
Wants=network-online.target
[Service]
Type=oneshot
RemainAfterExit=yes
User=$USER_NAME
ExecStart=$REPO/tests/strength/ec2/resume-gauntlet.sh
[Install]
WantedBy=multi-user.target
EOF
    systemctl daemon-reload
    systemctl enable resume-gauntlet.service
    echo "resume-gauntlet.service enabled for $USER_NAME in $REPO"
    exit 0
fi

active="$CHESS_DIR/results/active"
[[ -f "$active" ]] || { echo "no active gauntlet"; exit 0; }
if tmux has-session -t g 2>/dev/null; then
    echo "gauntlet session already running"
    exit 0
fi
echo "resuming: $(cat "$active")"
tmux new -d -s g "cd $(printf %q "$REPO") && $(cat "$active") 2>&1 | tee -a $(printf %q "$CHESS_DIR/results/resume.log")"
