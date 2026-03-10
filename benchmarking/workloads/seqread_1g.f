set $dir=tmp
set $filesize=1g
set $iosize=1m

define file name=microseqread,path=$dir,size=$filesize,prealloc

define process name=seqread,instances=1
{
    thread name=seqreader,memsize=10m,instances=1
    {
        flowop read name=read-file,filename=microseqread,iosize=$iosize,random=false
    }
}

run 10