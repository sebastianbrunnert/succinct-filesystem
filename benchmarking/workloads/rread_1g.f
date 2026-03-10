set $dir=tmp
set $filesize=1g
set $iosize=4k

define file name=microrread,path=$dir,size=$filesize,prealloc

define process name=randread,instances=1
{
    thread name=randreader,memsize=10m,instances=1
    {
        flowop read name=rand-read,filename=microrread,iosize=$iosize,random=true
    }
}

run 10