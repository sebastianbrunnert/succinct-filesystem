set $dir=tmp
set $nfiles=1000
set $meandirwidth=20
set $filesize=4k
set $appendsize=4k

define fileset name=appendsmall,path=$dir,size=$filesize,entries=$nfiles,dirwidth=$meandirwidth,prealloc=100

define process name=appender,instances=1
{
    thread name=appendthread,memsize=10m,instances=1
    {
        flowop openfile name=open1,filesetname=appendsmall,fd=1
        flowop appendfile name=append1,fd=1,iosize=$appendsize
        flowop closefile name=close1,fd=1
    }
}

echo "Append to Files Benchmark"
run 10