#!/bin/bash
#
# This application launches a GStreamer pipeline to show the RTP stream. In case you have packet loss (some lines don't apear to be drawn) do a
#   echo 2097152 > /proc/sys/net/core/rmem_default
# to increase the kernel buffer size
#
#gst-launch-1.0 udpsrc port="1234" caps = "application/x-rtp, media=(string)video, clock-rate=(int)90000, encoding-name=(string)RAW, sampling=(string)RGB, depth=(string)8, width=(string)720, height=(string)576, colorimetry=(string)BT601-5, payload=(int)127, ssrc=(uint)0, timestamp-offset=(uint)0, seqnum-offset=(uint)0" ! rtpvrawdepay ! videoconvert ! queue ! ximagesink sync=false
gst-launch-1.0 udpsrc port="1234" caps = "application/x-rtp, media=(string)video, clock-rate=(int)90000, encoding-name=(string)RAW, sampling=(string)RGB, depth=(string)8, width=(string)720, height=(string)576, colorimetry=(string)BT601-5, payload=(int)127, ssrc=(uint)0, timestamp-offset=(uint)0, seqnum-offset=(uint)0" ! rtpjitterbuffer ! rtpvrawdepay ! videoconvert ! queue ! ximagesink sync=false
