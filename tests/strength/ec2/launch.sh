#!/usr/bin/env bash
# Launch one EC2 instance for a gauntlet run. Costs money while it exists:
# run teardown.sh when the gauntlet is done.
#
# MARKET=spot (default): the spot request is persistent with interruption
# behaviour "stop", so if AWS reclaims capacity the instance (and its disk) is
# stopped rather than deleted, and comes back when capacity returns;
# ec2/resume-gauntlet.sh restarts fastchess from its config.json.
# MARKET=ondemand: about 3x the price but never interrupted. Use it when spot
# capacity is short (three interruptions in eight hours on 2026-11-07).
#
# Usage: tests/strength/ec2/launch.sh
# Env:   MARKET (spot|ondemand), REGION (us-east-1), INSTANCE_TYPE (c7a.4xlarge), MAX_PRICE (0.40 USD/h),
#        MAX_HOURS (72, the instance shuts itself down after this), PUBKEY (~/.ssh/id_ed25519.pub),
#        STATE (ec2/state.env; use another file per extra box, and pass the same STATE to teardown.sh)
set -euo pipefail

MARKET="${MARKET:-spot}"
REGION="${REGION:-us-east-1}"
INSTANCE_TYPE="${INSTANCE_TYPE:-c7a.4xlarge}"
MAX_PRICE="${MAX_PRICE:-0.40}"
MAX_HOURS="${MAX_HOURS:-72}"
PUBKEY="${PUBKEY:-$HOME/.ssh/id_ed25519.pub}"
NAME="boudica-gauntlet"
HERE="$(cd "$(dirname "$0")" && pwd)"
STATE="${STATE:-$HERE/state.env}"  # set STATE to run more than one box
aws() { command aws --region "$REGION" "$@"; }

[[ ! -f "$STATE" ]] || { echo "$STATE exists: an instance may already be running (run teardown.sh first)" >&2; exit 1; }

ami=$(aws ssm get-parameter \
    --name /aws/service/canonical/ubuntu/server/24.04/stable/current/amd64/hvm/ebs-gp3/ami-id \
    --query Parameter.Value --output text)

if ! aws ec2 describe-key-pairs --key-names "$NAME" >/dev/null 2>&1; then
    aws ec2 import-key-pair --key-name "$NAME" --public-key-material "fileb://$PUBKEY" >/dev/null
fi

sg=$(aws ec2 describe-security-groups --filters Name=group-name,Values="$NAME" \
    --query 'SecurityGroups[0].GroupId' --output text)
if [[ "$sg" == "None" ]]; then
    sg=$(aws ec2 create-security-group --group-name "$NAME" \
        --description "SSH for Boudica gauntlet runs" --query GroupId --output text)
fi
# Allow SSH only from this machine's current public IP
my_ip=$(curl -s https://checkip.amazonaws.com)
aws ec2 authorize-security-group-ingress --group-id "$sg" \
    --protocol tcp --port 22 --cidr "$my_ip/32" >/dev/null 2>&1 || true

# User data runs on first boot only, but a spot interruption stops and restarts
# the instance, which clears a scheduled shutdown. So install a service that
# re-arms the shutdown on every boot against a fixed deadline.
read -r -d '' user_data <<'EOF' || true
#!/bin/bash
echo $(( $(date +%s) + @MAX_HOURS@ * 3600 )) > /etc/deadline-epoch
cat > /usr/local/sbin/deadline-shutdown <<'SCRIPT'
#!/bin/bash
left=$(( ($(cat /etc/deadline-epoch) - $(date +%s)) / 60 ))
if (( left <= 0 )); then shutdown -h now; else shutdown -h +$left; fi
SCRIPT
chmod +x /usr/local/sbin/deadline-shutdown
cat > /etc/systemd/system/deadline-shutdown.service <<'UNIT'
[Unit]
Description=Shut down when the gauntlet time limit is reached
After=multi-user.target
[Service]
Type=oneshot
ExecStart=/usr/local/sbin/deadline-shutdown
[Install]
WantedBy=multi-user.target
UNIT
systemctl enable --now deadline-shutdown.service
EOF
user_data="${user_data//@MAX_HOURS@/$MAX_HOURS}"

market=()
case "$MARKET" in
    spot) market=(--instance-market-options "MarketType=spot,SpotOptions={MaxPrice=$MAX_PRICE,SpotInstanceType=persistent,InstanceInterruptionBehavior=stop}") ;;
    ondemand) ;;
    *) echo "MARKET must be spot or ondemand" >&2; exit 2 ;;
esac

instance=$(aws ec2 run-instances \
    --image-id "$ami" --instance-type "$INSTANCE_TYPE" \
    --key-name "$NAME" --security-group-ids "$sg" \
    ${market[@]+"${market[@]}"} \
    --block-device-mappings 'DeviceName=/dev/sda1,Ebs={VolumeSize=30,VolumeType=gp3,DeleteOnTermination=true}' \
    --user-data "$user_data" \
    --tag-specifications "ResourceType=instance,Tags=[{Key=Name,Value=$NAME}]" \
    --query 'Instances[0].InstanceId' --output text)

aws ec2 wait instance-running --instance-ids "$instance"
ip=$(aws ec2 describe-instances --instance-ids "$instance" \
    --query 'Reservations[0].Instances[0].PublicIpAddress' --output text)
request=$(aws ec2 describe-instances --instance-ids "$instance" \
    --query 'Reservations[0].Instances[0].SpotInstanceRequestId' --output text)

cat > "$STATE" <<EOF
REGION=$REGION
INSTANCE_ID=$instance
SPOT_REQUEST_ID=$request
PUBLIC_IP=$ip
LAUNCHED=$(date -u +%Y-%m-%dT%H:%MZ)
EOF
cat "$STATE"
cat <<EOF

Next (from the repo root):
  rsync -a --exclude .git --exclude build --exclude boudica ./ ubuntu@$ip:boudica/
  ssh ubuntu@$ip 'sudo boudica/tests/strength/setup-tools.sh --with-tb --with-sf10 --with-rust && sudo chown -R ubuntu /opt/chess'
  ssh ubuntu@$ip 'cd boudica && make -s CXX=g++ && tests/strength/scale.sh && tests/strength/build-anchors.sh A'
  ssh ubuntu@$ip 'tmux new -d -s g "cd boudica && tests/strength/gauntlet.sh ./boudica A blitz"'
Fetch results:  rsync -a ubuntu@$ip:/opt/chess/results/ tests/strength/results/
Stop paying:    tests/strength/ec2/teardown.sh
EOF
