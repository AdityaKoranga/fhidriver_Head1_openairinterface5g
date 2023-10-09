<table style="border-collapse: collapse; border: none;">
  <tr style="border-collapse: collapse; border: none;">
    <td style="border-collapse: collapse; border: none;">
      <a href="http://www.openairinterface.org/">
         <img src="./images/oai_final_logo.png" alt="" border=3 height=50 width=150>
         </img>
      </a>
    </td>
    <td style="border-collapse: collapse; border: none; vertical-align: center;">
      <b><font size = "5">OAI 7.2 Fronthaul Interface 5G SA Tutorial</font></b>
    </td>
  </tr>
</table>

**Table of Contents**

[[_TOC_]]

# 1. Prerequisites

The hardware on which we have tried this tutorial:

|Hardware (CPU,RAM)                          |Operating System                  |NIC (Vendor,Driver,Firmware)                     |
|--------------------------------------------|----------------------------------|-------------------------------------------------|
|Intel(R) Xeon(R) Gold 6154 (2*18 Core), 64GB|RHEL 8.6 (4.18.0-372.26.1.rt7.183)|QLogic FastLinQ QL41000,qede,mbi 15.35.1         |
|Intel(R) Xeon(R) Gold 6354 18-Core, 128GB   |RHEL 8.7 (4.18.0-425.10.1.rt7.220)|Intel XXV710 for 25GbE SFP28,i40e,6.02 0x80003888|
|AMD EPYC 7513 32-Core Processor, 256GB      |Ubuntu 20.04 (5.4.143-rt64)       |Intel X710 for 10GbE SFP+,i40e,5.04 0x80002530   |

**NOTE**: These are not minimum hardware requirements. This is the configuration of our servers.

We always set our servers to maximum performance mode. 
```bash
tuned-adm profile realtime
```

For PTP grandmaster we have used Fibrolan Falcon-RX. The O-RU which we have used for this tutorial is VVDN LPRU.

## 1.1 DPDK(Data Plane Development Kit)

Download DPDK version 20.05.0
```bash
wget http://fast.dpdk.org/rel/dpdk-20.05.tar.xz
```

DPDK Compilation
```bash
tar -xvf dpdk-20.05.tar.xz
cd dpdk-20.05

meson build
cd build
sudo ninja
sudo ninja install

make install T=x86_64-native-linuxapp-gcc
```

## 1.2 Setup

We have mentioned the information for our setup but if you want more information then you can refer to below links,

1. [O-RAN-SC O-DU Setup Configuration](https://docs.o-ran-sc.org/projects/o-ran-sc-o-du-phy/en/latest/Setup-Configuration_fh.html)
2. [O-RAN Cloud Platform Reference Designs 2.0,O-RAN.WG6.CLOUD-REF-v02.00,February 2021](https://orandownloadsweb.azurewebsites.net/specifications)

### 1.2.1 RHEL

These arguments we tried on both RHEL 8.6 (4.18.0-372.26.1.rt7.183.el8_6.x86_64) and 8.7 (4.18.0-425.10.1.rt7.220.el8_7.x86_64) 

Update Linux boot arguments
```bash
igb.max_vfs=2 intel_iommu=on iommu=pt intel_pstate=disable nosoftlockup tsc=nowatchdog mitigations=off cgroup_memory=1 cgroup_enable=memory mce=off idle=poll hugepagesz=1G hugepages=40 hugepagesz=2M hugepages=0 default_hugepagesz=1G selinux=0 enforcing=0 nmi_watchdog=0 softlockup_panic=0 audit=0 skew_tick=1 isolcpus=managed_irq,domain,0-2,8-17 nohz_full=0-2,8-17 rcu_nocbs=0-2,8-17 rcu_nocb_poll
```

### 1.2.1 Ubuntu

Install real timer kernel followed by updating boot arguments
```bash
isolcpus=0-2,8-17 nohz=on nohz_full=0-2,8-17 rcu_nocbs=0-2,8-17 rcu_nocb_poll nosoftlockup default_hugepagesz=1GB hugepagesz=1G hugepages=10 amd_iommu=on iommu=pt
```

Isolated CPU 0-2 are used for DPDK/ORAN and CPU 8 for `ru_thread` in our example config

## 1.3 PTP configuration

You can refer to the [following o-ran link](https://docs.o-ran-sc.org/projects/o-ran-sc-o-du-phy/en/latest/PTP-configuration_fh.html) for PTP configuration. In our setup we used Fibrolan Falcon-RX for PTP grandmaster. 
```bash
git clone http://git.code.sf.net/p/linuxptp/code linuxptp
git checkout v2.0
make && make install

./ptp4l -i ens1f1 -m -H -2 -s -f configs/default.cfg
./phc2sys -w -m -s ens1f1 -R 8 -f configs/default.cfg
```

# 2. Build OAI-FHI gNB

## 2.1 Build ORAN Fronthaul Interface Library

Download ORAN FHI library
```bash
git clone https://gerrit.o-ran-sc.org/r/o-du/phy.git
cd phy
git checkout oran_release_bronze_v1.1
```

Apply patches (available in `oai_folder/cmake_targets/tools/oran_fhi_integration_patches/`)
```bash
git apply oran-fhi-1-compile-libxran-using-gcc-and-disable-avx512.patch
git apply oran-fhi-2-return-correct-slot_id.patch
git apply oran-fhi-3-disable-pkt-validate-at-process_mbuf.patch
git apply oran-fhi-4-process_all_rx_ring.patch
git apply oran-fhi-5-remove-not-used-dependencies.patch
```

Set up the environment (change the path if you use different folders)

```bash
export XRAN_LIB_DIR=~/phy/fhi_lib/lib/build
export XRAN_DIR=~/phy/fhi_lib
export RTE_SDK=~/dpdk-20.05
export RTE_TARGET=x86_64-native-linuxapp-gcc
export RTE_INCLUDE=${RTE_SDK}/${RTE_TARGET}/include
```

Compile Fronthaul Interface Library
```bash
cd phy/fhi_lib/
./build.sh
```

## 2.2 Build OAI gNB

```bash
git clone https://gitlab.eurecom.fr/oai/openairinterface5g.git
cd openairinterface5g
git checkout develop
source oaienv
cd cmake_targets
./build_oai --gNB --ninja -t oran_fhlib_5g (Add, -I if you are building for the first time on server for installing external dependencies)
```

# 3. Configure Server and OAI gNB

## 3.1 Update following in fronthaul interface configuration - oran.fhi.json

```
 * DU Mac-address: Parameter o_du_macaddr 
 * RU MAC Address: Parameter o_ru_macaddr
 * PCI address: Parameter dpdk_dev_up and dpdk_dev_cp
```

## 3.2 Copy Fronthaul Configuration File

```bash
cd ran_build/build
cp ../../../targets/PROJECTS/GENERIC-NR-5GC/CONF/oran.fhi.json .
```

## 3.2 Bind Devices

```bash
echo "2" > /sys/class/net/ens1f1/device/sriov_numvfs
sudo ip link set ens1f1 vf 0 mac 00:11:22:33:44:66 spoofchk off
sudo ip link set ens1f1 vf 1 mac 00:11:22:33:44:66 spoofchk off
sudo modprobe vfio_pci
sudo /usr/local/bin/dpdk-devbind.py --bind vfio-pci 51:0e.0
sudo /usr/local/bin/dpdk-devbind.py --bind vfio-pci 51:0e.1
```

# 4. Run OAI gNB

## 4.1 Run OAI gNB

```bash
cd ran_build/build

sudo ./nr-softmodem -O ../../../targets/PROJECTS/GENERIC-NR-5GC/CONF/oran.fh.band78.fr1.273PRB.conf --sa --reorder-thread-disable
```
