### 环境实测

这里的内容是描述如何搭建linux debug 环境的。

#### 1. 搭建NFS的网络挂载环境

​		基础知识参考视频： https://www.yuanzige.com/course/detail/50096

* 首先要确定虚拟机和开发板要能ping通，我用的是WSL环境，参考这篇文章设置的`．wslconfig`和虚拟机的`/etc/wsl/conf`。如果不行的话，可以先将WSL 升级试试。

  文章链接：https://blog.gazer.win/essay/wsl2-mirrored-network.html

* 查看电脑ip地址，給开发板上电，利用if 命令设置开发板ipv4 的ip地址，然后关闭**电脑局域网防火墙**，此时WSL和开发板应该能相互Ping通

  ```powershell
  root@ATK-IMX6U:~# ifconfig eth0 192.168.19.10 netmask 255.255.255.0
  root@ATK-IMX6U:~# ifconfig
  eth0      Link encap:Ethernet  HWaddr 88:4a:d9:3c:c0:9f
            inet addr:192.168.19.10  Bcast:192.168.19.255  Mask:255.255.255.0
            inet6 addr: fe80::8a4a:d9ff:fe3c:c09f/64 Scope:Link
            UP BROADCAST RUNNING MULTICAST DYNAMIC  MTU:1500  Metric:1
            RX packets:204 errors:0 dropped:0 overruns:0 frame:0
            TX packets:66 errors:0 dropped:0 overruns:0 carrier:0
            collisions:0 txqueuelen:1000
            RX bytes:26159 (25.5 KiB)  TX bytes:20037 (19.5 KiB)
  
  root@ATK-IMX6U:~# ping 192.168.19.12
  PING 192.168.19.12 (192.168.19.12) 56(84) bytes of data.
  64 bytes from 192.168.19.12: icmp_seq=1 ttl=128 time=0.558 ms
  .....
  
  
  
  /* 电脑 */
  wxwang@LAPTOP-1GKOH5SA:~$ ifconfig
  eth0: flags=4163<UP,BROADCAST,RUNNING,MULTICAST>  mtu 1500
          inet 192.168.19.12  netmask 255.255.255.0  broadcast 192.168.19.255
          inet6 fe80::a21f:2810:8131:97fd  prefixlen 64  scopeid 0x20<link>
          ......
          
  wxwang@LAPTOP-1GKOH5SA:~$ ping 192.168.19.10
  PING 192.168.19.10 (192.168.19.10) 56(84) bytes of data.
  64 bytes from 192.168.19.10: icmp_seq=1 ttl=64 time=1.58 ms  
  ```

* 开发板和电脑搭建nfs文件系统

  * WSL 端

    ``` c
    /* 在 Ubuntu 终端执行以下指令安装 NFS。 */
    sudo apt-get install nfs-kernel-server 
        
    /* 新建 NFS 共享目录，并给予 NFS 目录可读可写可执行权限。 */
    sudo mkdir /home/wxwang/imax6ull/opendv/nfs
    sudo chmod 777 /home/wxwang/imax6ull/opendv/nfs
        
    /* 配置 NFS 服务  */
    sudo vi /etc/exports 
        最后一行添加：
        /home/wxwang/imax6ull/opendv/nfs *(rw,sync,no_root_squash,no_subtree_check)
        
    /* 重启 NFS 服务器。  */  
    sudo /etc/init.d/nfs-kernel-server restart 
    
    /* 查看 NFS 共享目录 */    
    showmount -e 
        
    /* 结果 */
    wxwang@LAPTOP-1GKOH5SA:~/imax6ull/opendv/nfs$ showmount -e
    Export list for LAPTOP-1GKOH5SA:
    /home/wxwang/imax6ull/opendv/nfs *
        
    /*建立个文件 用于确认nfs确实是挂载成功了*/
     cd /home/wxwang/imax6ull/nfs
     vim mytest.c /* 添加hello imax6ull */
    ```

  * borad 端，在下面的操作之前要先保证是能相互pin通的

    ```c
    /* 创建挂载的文件夹 */
    mkdir nfs
    /* 挂载 */
    mount -t nfs -o nolock,nfsvers=3 192.168.19.12:/home/wxwang/imax6ull/opendv/nfs nfs/
    /* 卸载 */
    umount nfs
    ```


  * **过程中遇到的问题：**在这个过程中，主要是WSL侧遇到的问题比较多

    * nfs -系统启动失败，并且有下面的log 提示。这个是/etc/exports 中设置时缺少了no_subtree_check 。在括号内添加就好

      ```powershell
      wxwang@LAPTOP-1GKOH5SA:~$ sudo /etc/init.d/nfs-kernel-server restart
       * Stopping NFS kernel daemon                                                                                                                [ OK ]
       * Unexporting directories for NFS kernel daemon...                                                                                          [ OK ]
       * Exporting directories for NFS kernel daemon...                                                                                                   exportfs: /etc/exports [3]: Neither 'subtree_check' or 'no_subtree_check' specified for export "*:/home/wxwang/imax6ull/opendv/nfs".
        Assuming default behaviour ('no_subtree_check').
        NOTE: this default has changed since nfs-utils version 1.0.x
      
                                                                                                                                                   [ OK ]
       * Starting NFS kernel daemon
       * Not starting: portmapper is not running
      ```

      解决方式：

      ```c
      /home/wxwang/imax6ull/opendv/nfs *(rw,sync,no_root_squash,no_subtree_check)
      ```

    * nfs-系统启动失败，并有下面的log。重点是`Not starting: portmapper is not running`启动不起来。

      ```powershell
      wxwang@LAPTOP-1GKOH5SA:~$ sudo /etc/init.d/nfs-kernel-server restart
       * Stopping NFS kernel daemon                                                                                                     [ OK ]
       * Unexporting directories for NFS kernel daemon...                                                                               [ OK ]
       * Exporting directories for NFS kernel daemon...                                                                                 [ OK ]
       * Starting NFS kernel daemon
       * Not starting: portmapper is not running
      ```

      之后查阅发现，是rpcbind 没有启动，于是想要启动rpcbind.

      ```powershell
      wxwang@LAPTOP-1GKOH5SA:~$ sudo service rpcbind restart
       * Stopping RPC port mapper daemon rpcbind                                                                                        [ OK ]
       * Starting RPC port mapper daemon rpcbind                                                                                               ln: failed to create symbolic link '/run/sendsigs.omit.d/rpcbind': No such file or directory
                                                                                                                                        [fatil]
      ```

      提示`/run/sendsigs.omit.d/rpcbind`这个文件没有。多次尝试其他的方式没有结果，问**GPT**，提示直接新建一个这个文件，并且赋予执行权限。尝试后解决。

      ```c
      sudo mkdir -p /run/sendsigs.omit.d
      sudo chmod 755 /run/sendsigs.omit.d
      sudo service rpcbind restart
      ```

    * NFS 启动失败，提示`Unit nfs.service could not be found.`

      ```powershell
      wxwang@LAPTOP-1GKOH5SA:~$ systemctl status nfs
      ERROR:systemctl:Unit nfs.service could not be found.
      ```

      

  

  

  

  

  

  

  
