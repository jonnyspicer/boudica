#!/usr/bin/env bash
# Cancel the persistent spot request and terminate the gauntlet instance.
# Cancelling first matters: terminating alone lets a persistent request launch
# a replacement. Fetch results before running this; the disk is deleted.
set -euo pipefail

HERE="$(cd "$(dirname "$0")" && pwd)"
STATE="$HERE/state.env"
[[ -f "$STATE" ]] || { echo "no $STATE: nothing to tear down" >&2; exit 1; }
source "$STATE"
aws() { command aws --region "$REGION" "$@"; }

aws ec2 cancel-spot-instance-requests --spot-instance-request-ids "$SPOT_REQUEST_ID" >/dev/null
aws ec2 terminate-instances --instance-ids "$INSTANCE_ID" >/dev/null
aws ec2 wait instance-terminated --instance-ids "$INSTANCE_ID"
rm "$STATE"
echo "Terminated $INSTANCE_ID and cancelled $SPOT_REQUEST_ID in $REGION"
