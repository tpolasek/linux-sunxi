if [ `env | grep PATH | grep linaro | wc -l` -ne "1" ]
then
  echo "Linaro not in path, adding"
  export PATH="$PATH":/home/thomas/c64/linux/linaro/bin
fi

make -j$(nproc) ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- uImage modules
make ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- INSTALL_MOD_PATH=output modules_install
