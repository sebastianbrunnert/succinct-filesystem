set $dir=tmp
set $nfiles=5000
set $meandirwidth=100
set $filesize=4k

define fileset name=deletesmall,path=$dir,size=$filesize,entries=$nfiles,dirwidth=$meandirwidth,prealloc=100

define process name=filedeleter,instances=1
{
    thread name=filedeleterthread,memsize=10m,instances=1
    {
        flowop deletefile name=delete1,filesetname=deletesmall
        flowop opslimit name=limit
        flowop finishoncount name=finish,value=$nfiles
    }
}

run 10
