set $dir=tmp
set $nfiles=5000
set $meandirwidth=100
set $filesize=4k
set $iosize=1m

set mode quit firstdone

define fileset name=createsmall,path=$dir,size=$filesize,entries=$nfiles,dirwidth=$meandirwidth

define process name=filecreator,instances=1
{
    thread name=filecreatorthread,memsize=10m,instances=1
    {
        flowop createfile name=create1,filesetname=createsmall,fd=1
        flowop writewholefile name=write1,srcfd=1,fd=1,iosize=$iosize
        flowop closefile name=close1,fd=1
    }
}

run 10