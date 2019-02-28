function embed(audio_filename,watermark_filename)
%%%Ç¶ÈëË®Ó¡
[m,fs]=audioread(audio_filename);
L=size(m);
p=imread(watermark_filename);
pw=im2bw(p);
[m1,m2]=size(pw);
 
n=10;
length=m1*m2*10;
i=1:length;
j=[1];
me=m(i,j);
i=(length+1):L;
mr=m(i,j);
k=1;
B=cell(m1*m2,1);
while(k<m1*m2*n)
    i=k:(k+9);
    s=(k+9)/10;
    B{s,1}=me(i,j);
  
    k=k+10;  
end
C=reshape(pw,1,m1*m2);
D=cell(m1*m2,1);
for i=1:m1*m2
    D{i,1}=dct(B{i,1});    
end
E=cell(m1*m2,1);
E=D;
for i=1:m1*m2
    E{i,1}(3)=D{i,1}(3)*(1+C(i));
end
F=cell(m1*m2,1);
for i=1:m1*m2
    F{i,1}=idct(E{i,1});
end
G=[F{1,1};F{2,1}];
for i=3:m1*m2
    G=[G;F{i,1}];
end
G=[G;mr];
audiowrite('MARKED.wav',G,fs);