#  CPA attack on FE310 risc-v silicon with Chipwhisperer

### Building and flashing
1. `(cd docker-riscv-toolchain && docker build -t riscv-toolchain .)`
2. `chmod +x run.sh && ./run.sh`
3. `make software PROGRAM=cw-riscv`
4. `make upload PROGRAM=cw-riscv`
