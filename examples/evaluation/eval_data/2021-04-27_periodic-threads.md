Eval done. Dumping logs now..
Printing measurements for type 1 (type Create Thread)
Duration ticks | Duration cycles | Description 
       172 [/home/fritz/git/github/sancus-riot/sancus-testbed/evaluation/main.c] Hi from periodic thread bar at 3618896544586..
    |        688      | CREATE fooD
       176     |        704      | CREATE fooE
       181     |        724      | CREATE fooF
       186     |        744      | CREATE fooG
       189     |        756      | CREATE fooH
       193     |        772      | CREATE fooI
       198     |        792      | CREATE fooJ
       202     |        808      | CREATE fooK
       206     |        824      | CREATE fooL
       212     |        848      | CREATE fooM
       214     |        856      | CREATE EVAL
Printing measurements for type 2 (type Switch to periodic)
Duration ticks | Duration cycles | Description 
       296     |       1184      | PERIODIC foo
       318     |       1272      | PERIODIC bar
[/home/fritz/git/github/sancus-riot/sancus-testbed/evaluation/main.c] Hi from periodic thread bar at 3618896544586..
Printing measurements for type 3 (type Context Exit)
Duration ticks | Duration cycles | Description 
       127     |        508      | MAIN
       144     |        576      | EXIT fooD
       147     |        588      | EXIT fooE
       151     |        604      | EXIT fooF
       154     |        616      | EXIT fooG
       157     |        628      | EXIT fooH
       160     |        640      | EXIT fooI
       161     |        644      | EXIT fooJ
       164     |        656      | EXIT fooK
       189     |        756      | EXIT fooL
4294908893     | 4294733684      | EXIT fooM
Printing measurements for type 4 (type Sleep)
[/home/fritz/git/github/sancus-riot/sancus-testbed/evaluation/main.c] Hi from periodic thread bar at 3623191511883..
Duration ticks | Duration cycles | Description 
     43943     |     175772      | SLEEP fooD
      4246     |      16984      | SLEEP fooE
     41225     |     164900      | SLEEP fooF
     62003     |     248012      | SLEEP fooG
4294963673     | 4294952804      | SLEEP fooH
       281     |       1124      | SLEEP fooI
       284     |       1136      | SLEEP fooJ
       288     |       1152      | SLEEP fooK
       292     |       1168      | SLEEP fooL
     20774     |      83096      | SLEEP fooM
[/home/fritz/git/github/sancus-riot/sancus-testbed/evaluation/main.c] Hi from periodic thread bar at 3618896544586..
       337     |       1348      | SMfooC
Printing measurements for type 5 (type Yield)
Duration ticks | Duration cycles | Description 
       106     |        424      | YIELD fooD
       108     |        432      | YIELD fooE
       109     |        436      | YIELD fooF
       446     |       1784      | PER_SMfoo
       356     |       1424      | PER_SMbar
       111     |        444      | YIELD fooG
       339     |       1356      | PER_SMfoo
       112     |        448      | YIELD fooH
       114     |        456      | YIELD fooI
       421     |       1684      | PER_SMfoo
       116     |        464      | YIELD fooJ
       117     |        468      | Y[/home/fritz/git/github/sancus-riot/sancus-testbed/evaluation/main.c] Hi from periodic thread bar at 3584536806210..
IELD fooK
       119     |        476      | YIELD fooL
       139     |        556      | SMfooC
       152     |        608      | YIELD fooM
4294949675     | 4294896812      | PER_SMbar
4294949675     | 4294896812      | PER_SMbar
4294949644     | 4294896688      | PER_SMbar
4294949667     | 4294896780      | PER_SMbar
4294686812     | 4293845360      | PER_SMbar
[/home/fritz/git/github/sancus-riot/sancus-testbed/evaluation/main.c] Hi from periodic thread bar at 3618896544586..
Printing measurements for type 6 (type Get time)
Duration ticks | Duration cycles | Description 
        53     |        212      | TIME fooD
        53     |        212      | TIME fooE
        53     |        212      | TIME fooF
        53     |        212      | TIME fooG
        83     |        332      | PER_SMfoo
        83     |        332      | PER_SMbar
        53     |        212      | TIME fooH
        83     |        332      | PER_SMfoo
        53     |        212      | TIME fooI
        53     |        212      | TIME fooJ
        83     |        332      | PER_SMfoo
        83     |        332      | PER_SMbar
        53     |        212      | TIME fooK
        53     |        212      | TIME fooL
[/home/fritz/git/github/sancus-riot/sancus-testbed/evaluation/main.c] Hi from periodic thread bar at 3623191511883..
        89     |        356      | SMfooC
        83     |        332      | PER_SMbar
        53     |        212      | TIME fooM
        83     |        332      | PER_SMbar
        83     |        332      | PER_SMbar
        83     |        332      | PER_SMbar
        83     |        332      | PER_SMbar
        83     |        332      | PER_SMbar
        83     |        332      | PER_SMbar
        83     |        332      | PER_SMbar

