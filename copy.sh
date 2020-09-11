sudo cp arch/arm/boot/uImage /media/thomas/824bc10d-45ef-4910-a3fd-d6caa794ac63/uImage
sudo rm -rf /media/thomas/824bc10d-45ef-4910-a3fd-d6caa794ac63/lib/modules/3.4.104*
sudo cp -Rp output/lib/modules/3.4.104-gd47d3670* /media/thomas/824bc10d-45ef-4910-a3fd-d6caa794ac63/lib/modules/
sudo cp -Rp output/lib/modules/3.4.104-gd47d3670* /media/thomas/49b06f36-a3d5-4409-9671-33b3733244e8/lib/modules/
sync
sudo umount /media/thomas/824bc10d-45ef-4910-a3fd-d6caa794ac63/
sudo umount /media/thomas/49b06f36-a3d5-4409-9671-33b3733244e8/
