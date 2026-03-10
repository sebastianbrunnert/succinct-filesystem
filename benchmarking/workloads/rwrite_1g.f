set $dir=tmp
set $filesize=1g
set $iosize=4k

define file name=microrwrite,path=$dir,size=$filesize,prealloc

define process name=randwrite,instances=1
{
    thread name=randwriter,memsize=10m,instances=1
    {
        flowop write name=rand-write,filename=microrwrite,iosize=$iosize,random=true
    }
}

run 10