# Macsim 
## CXL Branch Usage
- Quick start
```{bash}
git clone https://github.com/snu-comparch/macsim.git
git checkout cxl-dev
git submodule init
git submodule update src/ramulator
cd scripts
./knobgen.pl
./statgen.pl
cd ..
cp src/ramulator/configs/* ./bin

git submodule update src/CXLSim
cd src/CXLSim
cd scripts
./knobgen.pl
./statgen.pl
cd ../../../
cp src/CXLSim/bin/cxl_params.in ./bin

./build.py --ramulator --cxl
```

## Generating traces for NDP
`pin -t ./tools/x86_trace_generator/obj-intel64/trace_generator.so -manual 1 -roi 1 -- <binary>`

## Integrating NDP into CXLSim & Macsim
- Trace generation
  - Mark ROI for traces (Done)
- Core
  - When the first instruction of ROI is fetched? (Done)
    - Dispatch them to CXLSim
    - For these instructions, don't consider dependency w.r.t instructions preceding ROI & succeeding ROI
  - Separate allocation queue (Done)
  - Retire them after NDP uses them (Done)
- CXLSim
  - Adding CXL latency to offload uops hinders the performance significantly
    - Implement uop offloading so that their offloading latency is neglected? (Done)
      - Added knob in CXLSim that can control this (KNOB_UOP_DIRECT_OFFLOAD)
      - If this knob is turned on, it will obtain/send uops from/to the external simulator w/o adding PCIe latency
      - Otherwise, offloading uops themselves adds overhead
    - After all, we are considering accelerators so only a initial signal is all that is required
- Ramulator : Change ramulator latency for cxl t3 device
  - Can we assume that instead of burst modes, we can just get 8B of data for every memory access inst? (Done)
    (since we are dealing with workloads with low locality)
- Currently IPC increases for NDP when cache sizes are small enough to make the workload BW bound

## TODO
- CXLSim & NDP
  - Add ports to model execution unit latency & BW
  - This NDP model is closer to a general RISC-V core rather than a specific accelerator
    - Should we constrain this device to be a simple in-order core?
  - To utilize the DRAM BW more efficiently (for instance, getting 8B of data for every mem access),
    we need to add compute units 'inside' DIMM
    - This is effective when the DRAM bw is the bottleneck, not the PCIe bw
  - Add caches inside NDP???
  - Maybe for this NDP, we don't need to touch the internal DRAM structure as we want to get over the PCIe BW bottleneck

## Introduction

* MacSim is a heterogeneous architecture timing model simulator that is
  developed from Georgia Institute of Technology.
* It simulates x86, ARM64, NVIDIA PTX and Intel GEN GPU instructions and can be configured as
  either a trace driven or execution-drive cycle level simulator. It models
  detailed mico-architectural behaviors, including pipeline stages,
  multi-threading, and memory systems.
* MacSim is capable of simulating a variety of architectures, such as Intel's
  Sandy Bridge, Skylake (both CPUs and GPUs) and NVIDIA's Fermi. It can simulate homogeneous ISA multicore
  simulations as well as heterogeneous ISA multicore simulations. It also
  supports asymmetric multicore configurations (small cores + medium cores + big
  cores) and SMT or MT architectures as well.
* Currently interconnection network model (based on IRIS) and power model (based
  on McPAT) are connected.
* MacSim is also one of the components of SST, so muiltiple MacSim simulatore
  can run concurrently.
* The project has been supported by Intel, NSF, Sandia National Lab.

## Note

* If you're interested in the Intel's integrated GPU model in MacSim, please refer to [intel_gpu](https://github.com/gthparch/macsim/tree/intel_gpu) branch.

* We've developed a power model for GPU architecture using McPAT. Please refer
  to the following paper for more detailed
  information. [Power Modeling for GPU Architecture using McPAT](http://www.cercs.gatech.edu/tech-reports/tr2013/git-cercs-13-10.pdf)
  Modeling for GPU Architecture using McPAT.pdf) by Jieun Lim, Nagesh
  B. Lakshminarayana, Hyesoon Kim, William Song, Sudhakar Yalamanchili, Wonyong
  Sung, from Transactions on Design Automation of Electronic Systems (TODAES)
  Vol. 19, No. 3.
* We've characterised the performance of Intel's integrated GPUs using MacSim. Please refer to the following paper for more detailed information. [Performance Characterisation and Simulation of Intel's Integrated GPU Architecture (ISPASS'18)](http://comparch.gatech.edu/hparch/papers/gera_ispass18.pdf)

## Intel GEN GPU Architecture
* Intel GEN9 GPU Architecture: ![](http://comparch.gatech.edu/hparch/images/intel_gen9_arch.png)

## Documentation

Please see [MacSim documentation file](https://github.com/gthparch/macsim/blob/master/doc/macsim.pdf) for more detailed descriptions.


## Download

* You can download the latest copy from our git repository.

```
git clone -b intel_gpu https://github.com/gthparch/macsim.git

download traces 
/macsim/tools/download_trace_files.py
```
## build 
  ./build.py --ramulator 
  (please see /macsim/INSTALL)

## People

* Prof. Hyesoon Kim (Project Leader) at Georgia Tech 
Hparch research group 
(http://hparch.gatech.edu/people.hparch) 



## Q & A 

If you have a question, please use github issue ticket. 


## Tutorial

* We had a tutorial in HPCA-2012. Please visit [here](http://comparch.gatech.edu/hparch/OcelotMacsim_tutorial.html) for the slides.
* We had a tutorial in ISCA-2012, Please visit [here](http://comparch.gatech.edu/hparch/isca12_gt.html) for the slides.


## SST+MacSim

* Here are two example configurations of SST+MacSim.
  * A multi-socket system with cache coherence model: ![](http://comparch.gatech.edu/hparch/images/sst+macsim_conf_1.png)
  * A CPU+GPU heterogeneous system with shared memory: ![](http://comparch.gatech.edu/hparch/images/sst+macsim_conf_2.png)


## Notes
```
mkdir build
cd build
cmake ..
make -j<cores>
make install
```
