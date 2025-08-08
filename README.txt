A HTTP Wrapper for reading FSUIPC simulator variables, written in C.
Feature: efficient, minimal, taking ~1MB of RAM and ~70ms response time (depending on the request).
Usage: See provided DemoRequest.txt and DemoResponse.txt, initially for FS2020.

This one is dedicated for classic 32-Bit simulators. There may be fewer offsets available for reading.
For modern simulators (64 Bit), see this repository -> https://github.com/Xuan0876/FSUIPC-HTTP-Wrapper-Server

Credit:
Moongoose Web Server - HTTP handling 
CJson - JSON handling
FSUIPC - initial program that this project was modified from is "64-bit External User kit for C/C++ on its own UIPC64 SDK C" taken from https://forum.simflight.com/topic/66136-useful-additional-programs/, as well as 32-Bit SDK taken from https://www.fsuipc.com/.
