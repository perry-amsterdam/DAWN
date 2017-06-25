# DAWN
Decentralized WiFi Controller

## Related Repositories

|Repro             |Content                   |
|------------------|--------------------------|
|[patches-pending](https://github.com/berlin-open-wireless-lab/patches-pending)|Pending LEDE / OpenWrt Patches DAWN is depending on|
|[bowl-feed](https://github.com/berlin-open-wireless-lab/bowl-feed)|Feed for DAWN|

## Installation

### Compiling LEDE with patch

Clone LEDE source code

    git clone https://github.com/lede-project/source.git

Switch to directory

    cd lede/
   
Clone patch

    git clone https://github.com/berlin-open-wireless-lab/patches-pending.git
    
Apply patches

    quilt apply
    
Updating feeds

    ./scripts/feeds update -a && ./scripts/feeds/install -a

Configure image

    make menuconfig

Compile image

    make -j $(nproc)

### Compiling DAWN

Add [bowlfeed](https://github.com/berlin-open-wireless-lab/bowl-feed.git) to feeds.conf  
    
    src-git bowlfeed git@github.com:berlin-open-wireless-lab/bowl-feed.git
    
Slect dawn under

    make menuconfig
    
Compile

    make package/dawn/compile
    
