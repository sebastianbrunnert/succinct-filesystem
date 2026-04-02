set $dir=tmp
set $filesize=1g
set $iosize=1m

define file name=microseqwrite,path=$dir,size=$filesize,prealloc

define process name=seqwrite,instances=1
{
    thread name=seqwriter,memsize=10m,instances=1
    {
        flowop write name=write-file,filename=microseqwrite,iosize=$iosize,random=false
    }
}

run 10