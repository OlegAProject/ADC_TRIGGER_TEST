delete(instrfind);
dat = serial('COM7', 'BaudRate', 115200);
dat.InputBufferSize = 4096;

fopen(dat)
set(dat, 'ByteOrder', 'littleEndian')
disp 'Ok!';
while 1
fwrite(dat, 1, 'uint8')
number = fread(dat, 1, 'uint16')
pause(0.1);
clc;
end
fclose(dat);
