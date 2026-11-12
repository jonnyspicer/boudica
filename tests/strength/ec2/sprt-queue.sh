#!/usr/bin/env bash
# Run queued SPRTs one after another on the test box, so tests keep going
# between check-ins. Start once in tmux:
#   tmux new -d -s q boudica/tests/strength/ec2/sprt-queue.sh
# A job leaves the queue only when it finishes, so after a spot interruption
# the same test restarts from scratch. To start the runner on every boot:
#   (crontab -l; echo "@reboot tmux new -d -s q $HOME/boudica/tests/strength/ec2/sprt-queue.sh") | crontab -
# Queue a test by appending a line of sprt-ref.sh arguments to $QUEUE:
#   echo "origin/my-branch origin/main 0 5" >> ~/sprt-queue.txt
# Each finished test appends "<args> -> <result dir>" plus fastchess's last
# SPRT verdict line to $DONE.
set -uo pipefail

QUEUE="${QUEUE:-$HOME/sprt-queue.txt}"
DONE="${DONE:-$HOME/sprt-done.txt}"
CHESS_DIR="${CHESS_DIR:-/opt/chess}"
HERE="$(cd "$(dirname "$0")" && pwd)"
touch "$QUEUE" "$DONE"

while true; do
    job=$(head -n 1 "$QUEUE")
    if [[ -z "$job" ]]; then sleep 30; continue; fi
    echo "$(date -u +%FT%TZ) start: $job"
    # shellcheck disable=SC2086  # job is a list of arguments
    "$HERE/sprt-ref.sh" $job > "$HOME/sprt-current.log" 2>&1
    # Drop the finished job (match the line, in case the queue was edited meanwhile)
    grep -vxF -- "$job" "$QUEUE" > "$QUEUE.tmp" || true
    mv "$QUEUE.tmp" "$QUEUE"
    out=$(ls -dt "$CHESS_DIR"/results/sprt-* | head -1)
    verdict=$(grep -E "SPRT test (accepted|rejected)|H0 was accepted|H1 was accepted" "$out/fastchess.log" | tail -1)
    elo=$(grep -E "^Elo:" "$out/fastchess.log" | tail -1)
    echo "$(date -u +%FT%TZ) $job -> $out | ${verdict:-no verdict} | $elo" | tee -a "$DONE"
done
