#include <OSCBundle.h>
#include <OSCMessage.h>
#include <SLIPEncodedSerial.h>

#include <stdint.h>

#include "config.h"
#include "helpers.h"

#include "teensyaudio.h"

#ifdef USE_EEPROM

#include "storage.h"

#endif

#ifdef USE_DISPLAY

#include "display.h"

#endif

ChanInfo channel_info[] = {
    {CHAN_WHITE, "1", "IN1", 0},      {CHAN_WHITE, "2", "IN2", 0},
    {CHAN_WHITE, "3", "IN3", 0},      {CHAN_YELLOW, "P", "PC", 0},
    {CHAN_MAGENTA, "USB", "USB1", 1}, {CHAN_MAGENTA, "USB", "USB2", 2},

    {CHAN_WHITE, "1", "OUT1", 0},     {CHAN_WHITE, "2", "OUT2", 0},
    {CHAN_GREEN, "AFL", "HP1", 1},    {CHAN_GREEN, "AFL", "HP2", 2},
    {CHAN_MAGENTA, "USB", "USB1", 1}, {CHAN_MAGENTA, "USB", "USB2", 2},
};

#ifdef USE_DISPLAY
ST7735_t3 display = ST7735_t3(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCK, TFT_RST);
#endif

SLIPEncodedUSBSerial slip(Serial);

void onOscChannel(OSCMessage &msg, int patternOffset) {
    char buf[12];
    char address[22];
    int channel = -1;
    int addr;
    int offset;

    // /ch/<num>
    for (int i = 0; i < 6; i++) {
        sprintf(buf, "/%d", i);
        offset = msg.match(buf, patternOffset);
        if (offset) {
            channel = i;
            addr = offset + patternOffset;
            break;
        }
    }
    if (channel < 0)
        return;

    if (msg.match("/config/name", addr) > 0) {
        if (msg.isString(0)) {
            msg.getString(0, channel_info[channel].desc,
                          sizeof(channel_info[channel].desc));
        } else {
            snprintf(address, 22, "/ch/%d/config/name", channel);
            OSCMessage response(address);
            response.add(channel_info[channel].desc);
            slip.beginPacket();
            response.send(slip);
            slip.endPacket();
        }
        return;
    } else if (msg.match("/levels", addr) > 0) {
        OSCBundle response;
        Levels &levels = audio_get_levels();

        snprintf(address, 22, "/ch/%d/levels/rms", channel);
        response.add(address).add(rmsToDb(levels.rms[channel]));

        snprintf(address, 22, "/ch/%d/levels/peak", channel);
        response.add(address).add(rmsToDb(levels.peak[channel]));

        snprintf(address, 22, "/ch/%d/levels/smooth", channel);
        response.add(address).add(rmsToDb(levels.smooth[channel]));

        slip.beginPacket();
        response.send(slip);
        slip.endPacket();
        return;
    } else if (msg.match("/multiplier", addr) > 0) {
        if (msg.isFloat(0))
            set_channel_multiplier(channel, msg.getFloat(0));
        else {
            snprintf(address, 22, "/ch/%d/multiplier", channel);
            OSCMessage response(address);
            response.add(get_channel_multiplier(channel));
            slip.beginPacket();
            response.send(slip);
            slip.endPacket();
        }
    }

    // /ch/<num>/mix
    offset = msg.match("/mix", addr);
    addr += offset;
    if (offset < 1)
        return;

    // /ch/<num>/mix/<bus>/level
    int bus = -1;
    for (int i = 0; i < 6; i++) {
        sprintf(buf, "/%d/level", i);
        int offset = msg.match(buf, addr);
        if (offset) {
            bus = i;
            break;
        }
    }
    if (bus < 0)
        goto mutes;

    if (msg.isFloat(0))
        set_gain(channel, bus, msg.getFloat(0));
    else {
        snprintf(address, 22, "/ch/%d/mix/%d/level", channel, bus);
        OSCMessage response(address);
        response.add((float)get_gain(channel, bus));
        slip.beginPacket();
        response.send(slip);
        slip.endPacket();
    }

mutes:
    for (int i = 0; i < 6; i++) {
        sprintf(buf, "/%d/muted", i);
        int offset = msg.match(buf, addr);
        if (offset) {
            bus = i;
            break;
        }
    }
    if (bus < 0)
        return;

    if (msg.isBoolean(0)) {
        if (msg.getBoolean(0))
            unmute(channel, bus);
        else
            mute(channel, bus);
    } else {
        snprintf(address, 22, "/ch/%d/mix/%d/muted", channel, bus);
        OSCMessage response(address);
        response.add(is_muted(channel, bus));
        slip.beginPacket();
        response.send(slip);
        slip.endPacket();
    }
}

void onOscInfo(OSCMessage &msg) {
    char addrbuf[22];
    OSCBundle info;

    int i;

    info.add("/info/buses").add((uint8_t)BUSES);
    info.add("/info/channels").add((uint8_t)CHANNELS);
    info.add("/info/features").add(FEATURES);

    for (i = 0; i < BUSES; ++i) {
        snprintf(addrbuf, 22, "/bus/%d/config/name", i);
        info.add(addrbuf).add(channel_info[CHANNELS + i].desc);
    }
    for (i = 0; i < CHANNELS; ++i) {
        snprintf(addrbuf, 22, "/ch/%d/config/name", i);
        info.add(addrbuf).add(channel_info[i].desc);
    }

    slip.beginPacket();
    info.send(slip);
    slip.endPacket();
}

void onOscBus(OSCMessage &msg, int patternOffset) {
    char buf[12];
    char address[22];
    int bus = -1;
    int addr;
    int offset;

    // /bus/<num>
    for (int i = 0; i < 6; i++) {
        sprintf(buf, "/%d", i);
        offset = msg.match(buf, patternOffset);
        if (offset) {
            bus = i;
            addr = offset + patternOffset;
            break;
        }
    }
    if (bus < 0)
        return;

    if (msg.match("/config/name", addr) > 0) {
        if (msg.isString(0)) {
            msg.getString(0, channel_info[CHANNELS + bus].desc,
                          sizeof(channel_info[CHANNELS + bus].desc));
        } else {
            snprintf(address, 22, "/bus/%d/config/name", bus);
            OSCMessage response(address);
            response.add(channel_info[CHANNELS + bus].desc);
            slip.beginPacket();
            response.send(slip);
            slip.endPacket();
        }
        return;
    } else if (msg.match("/levels", addr) > 0) {
        OSCBundle response;
        Levels &levels = audio_get_levels();

        snprintf(address, 22, "/bus/%d/levels/rms", bus);
        response.add(address).add(rmsToDb(levels.rms[CHANNELS + bus]));

        snprintf(address, 22, "/bus/%d/levels/peak", bus);
        response.add(address).add(rmsToDb(levels.peak[CHANNELS + bus]));

        snprintf(address, 22, "/bus/%d/levels/smooth", bus);
        response.add(address).add(rmsToDb(levels.smooth[CHANNELS + bus]));

        slip.beginPacket();
        response.send(slip);
        slip.endPacket();
    } else if (msg.match("/multiplier", addr) > 0) {
        if (msg.isFloat(0))
            set_bus_multiplier(bus, msg.getFloat(0));
        else {
            snprintf(address, 22, "/bus/%d/multiplier", bus);
            OSCMessage response(address);
            response.add(get_bus_multiplier(bus));
            slip.beginPacket();
            response.send(slip);
            slip.endPacket();
        }
    }
}

void onPacketReceived(OSCMessage msg) {
    msg.route("/ch", onOscChannel);
    msg.route("/bus", onOscBus);
    msg.dispatch("/info", onOscInfo);
}

void setup() {
#ifdef USE_DISPLAY
    display_setup(display);
#endif
    audio_load_state();

    audio_setup();

    slip.begin(115200);
}

unsigned long last_draw = 0;

void loop() {
    int size;
    OSCMessage msg;
    if (slip.available()) {
        while (!slip.endofPacket()) {
            if ((size = slip.available()) > 0) {
                while (size--)
                    msg.fill(slip.read());
            }
        }
        if (!msg.hasError()) {
            onPacketReceived(msg);
        }
    }

    Levels &levels = audio_get_levels();

    audio_update_levels(levels);

#ifdef USE_DISPLAY
    update_display(display, levels.rms, channel_info);

    if (last_draw < (millis() - 16)) {
        display.updateScreen();
        last_draw = millis();
    }
#endif
}

int main() {
    setup();
    while (1) {
        loop();
    }
};
