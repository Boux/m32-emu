# tools/screenshot.sh

Renders the module in a real Rack instance under Xvfb and saves a screenshot, using a throwaway
Rack user directory so your own Rack configuration is untouched.

    make install
    mkdir -p /tmp/m32-user/plugins-lin-x64 && cp dist/*.vcvplugin /tmp/m32-user/plugins-lin-x64/
    echo '{}' > /tmp/m32-user/settings.json
    tools/screenshot.sh /tmp/m32-user out.png patch.vcv 8

Rack shows a "crashed during the last session" prompt whenever it is not shut down through its own
UI, and blocks on it. The script dismisses that prompt by killing only Rack's own zenity child.
