A HTTP Wrapper for reading FSUIPC simulator variables, written in C.
Feature: efficient, minimal, taking ~1MB of RAM and <10 ms response time (depending on the request and simulator, for FSX the response time could be ~70ms).
Usage: See provided DemoRequest.txt and DemoResponse.txt, made for MSFS2020.

This one is not limited to classic 32-Bit simulators. We have tested it on FSX, MSFS2020/2024, and X-Plane with XUIPC. 
However, for different simulators, the offsets may be different.


Credit:
Moongoose Web Server - HTTP handling 
CJson - JSON handling
FSUIPC - initial program that this project was modified from is "64-bit External User kit for C/C++ on its own UIPC64 SDK C" taken from https://forum.simflight.com/topic/66136-useful-additional-programs/, as well as 32-Bit SDK taken from https://www.fsuipc.com/.
