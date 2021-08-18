######## Riot on Sancus
Cycle accurate riot evaluation
Remember to set clock divider to 1 or multiply cycles accordingly
Clock divider is currently set to 4.
New SM 2 config: c544 c96c 0c72 0da0, 0
Vendor key: 4078d505d82099ba
SM key: e13efcd27df325f8
SM foo with ID 2 enabled	: 0xc544 0xc96c 0x0c72 0x0da0
SM foo done.[/home/fritz/git/github/sancus-riot/sancus-testbed/evaluation/main.c] Initializing SM foo with PID..
New SM 3 config: a7cc abf4 032e 045c, 0
Vendor key: 4078d505d82099ba
SM key: a312402605a426e7
SM bar with ID 3 enabled	: 0xa7cc 0xabf4 0x032e 0x045c
SM bar done.[/home/fritz/git/github/sancus-riot/sancus-testbed/evaluation/main.c] Initializing SM bar with PID..
New SM 4 config: a28c a7cc 0200 032e, 0
Vendor key: 4078d505d82099ba
SM key: 8132cbf4598dfff6
SM fooC with ID 4 enabled	: 0xa28c 0xa7cc 0x0200 0x032e
SM fooC done.[/home/fritz/git/github/sancus-riot/sancus-testbed/evaluation/main.c] Initializing SM fooC with PID..
All threads have been started.
Exiting main thread
fooD is sleepy at 4784...
fooE is sleepy at 54400...
fooF is sleepy at 65301...
fooG is sleepy at 113476...
fooH is sleepy at 183198...
fooI is sleepy at 252941...
fooJ is sleepy at 261204...
fooK is sleepy at 269594...
fooL is sleepy at 277872...
[/home/fritz/git/github/sancus-riot/sancus-testbed/evaluation/main.c] Hi from periodic thread foo at 103305053167092..
[/home/fritz/git/github/sancus-riot/sancus-testbed/evaluation/main.c] Hi from periodic thread bar at 189436342283338..
[/home/fritz/git/github/sancus-riot/sancus-testbed/evaluation/main.c] Sleeping fooC at 275739430025928..
fooM is sleepy at 346218...
Eval done. Dumping logs now..
Printing measurements for type 1 (type Create Thread)
Duration ticks | Duration cycles | Description 
       172     |        688      | CREATE fooD
       176     |        704      | CREATE fooE
       181     |        724      | CREATE fooF
       187     |        748      | CREATE fooG
       188     |        752      | CREATE fooH
       193     |        772      | CREATE fooI
       198     |        792      | CREATE fooJ
       202     |        808      | CREATE fooK
       206     |        824      | CREATE fooL
       212     |        848      | CREATE fooM
       215     |        860      | CREATE EVAL
Printing measurements for type 2 (type Switch to periodic)
Duration ticks | Duration cycles | Description 
Printing measurements for type 3 (type Context Exit)
Duration ticks | Duration cycles | Description 
       128     |        512      | MAIN
       144     |        576      | EXIT fooD
       146     |        584      | EXIT fooE
       152     |        608      | EXIT fooF
       154     |        616      | EXIT fooG
       157     |        628      | EXIT fooH
       159     |        636      | EXIT fooI
       161     |        644      | EXIT fooJ
       164     |        656      | EXIT fooK
       184     |        736      | EXIT fooL
       183     |        732      | PER_SMbar
       154     |        616      | PER_SMfoo
4294591674     | 4293464808      | EXIT fooM
Printing measurements for type 4 (type Sleep)
Duration ticks | Duration cycles | Description 
     43943     |     175772      | SLEEP fooD
      4099     |      16396      | SLEEP fooE
     41225     |     164900      | SLEEP fooF
     61776     |     247104      | SLEEP fooG
     61779     |     247116      | SLEEP fooH
       281     |       1124      | SLEEP fooI
       284     |       1136      | SLEEP fooJ
       288     |       1152      | SLEEP fooK
       292     |       1168      | SLEEP fooL
     20795     |      83180      | SLEEP fooM
       330     |       1320      | SMfooC
Printing measurements for type 5 (type Yield)
Duration ticks | Duration cycles | Description 
       106     |        424      | YIELD fooD
       107     |        428      | YIELD fooE
       109     |        436      | YIELD fooF
       111     |        444      | YIELD fooG
       112     |        448      | YIELD fooH
       114     |        456      | YIELD fooI
       115     |        460      | YIELD fooJ
       117     |        468      | YIELD fooK
       119     |        476      | YIELD fooL
       157     |        628      | PER_SMfoo
       161     |        644      | PER_SMbar
       139     |        556      | SMfooC
       148     |        592      | YIELD fooM
Printing measurements for type 6 (type Get time)
Duration ticks | Duration cycles | Description 
        53     |        212      | TIME fooD
        53     |        212      | TIME fooE
        53     |        212      | TIME fooF
        53     |        212      | TIME fooG
        53     |        212      | TIME fooH
        53     |        212      | TIME fooI
        53     |        212      | TIME fooJ
        53     |        212      | TIME fooK
        53     |        212      | TIME fooL
        83     |        332      | PER_SMfoo
        83     |        332      | PER_SMbar
        88     |        352      | SMfooC
        53     |        212      | TIME fooM