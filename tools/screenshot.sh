#!/bin/bash
U="$1"; OUT="$2"; PATCH="$3"; WAIT="${4:-14}"
python3 - "$U" <<'PY'
import json, sys
p = sys.argv[1] + "/settings.json"
s = json.load(open(p))
s["skipLoadOnLaunch"] = False
s["showTipsOnLaunch"] = False
s["tipIndex"] = -1
json.dump(s, open(p, "w"), indent=2)
PY
cd ~/STUFF/_apps/Rack2Free || exit 1
LD_LIBRARY_PATH=. ./Rack -u "$U" ${PATCH:+"$PATCH"} &
RACK_PID=$!

# osdialog blocks on a zenity child. Dismiss only our own dialogs so Rack keeps loading.
for i in $(seq 1 10); do
  sleep 1
  pkill -P "$RACK_PID" -x zenity 2>/dev/null && echo "dismissed a dialog"
done

sleep "$WAIT"
scrot -o -F "$OUT" 2>/dev/null
kill "$RACK_PID" 2>/dev/null
wait "$RACK_PID" 2>/dev/null
echo "done"
