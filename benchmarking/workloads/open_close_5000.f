set $dir=tmp
set $nfiles=5000
set $meandirwidth=50
set $filesize=4k

define fileset name=openclose,path=$dir,size=$filesize,entries=$nfiles,dirwidth=$meandirwidth,prealloc=100

define process name=openclose,instances=1
{
    thread name=openclosethread,memsize=10m,instances=1
    {
        flowop openfile name=open1,filesetname=openclose,fd=1
        flowop closefile name=close1,fd=1
    }
}

run 10
