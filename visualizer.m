clc; clear; close all;
% Lectura de los archivos de reportes
seq = load("-ascii","sequential.txt");
hp = load("-ascii","heavy_process.txt");
php = load("-ascii","pre_heavy_process.txt");

% Tamano de la cantidad de muestras
[~,n1] = size(seq);
[~,n2] = size(hp);
[~,n3] = size(php);

% Separar cantidad de solicitudes y tiempo
seq_req  = [];
seq_time = [];
for i = 1:2:n1
  seq_req  = [seq_req seq(i)];
  seq_time = [seq_time seq(i+1)];
end  

hp_req  = [];
hp_time = [];
for i = 1:2:n2
  hp_req  = [hp_req hp(i)];
  hp_time = [hp_time hp(i+1)];
end  

php_req  = [];
php_time = [];
for i = 1:2:n3
  php_req  = [php_req php(i)];
  php_time = [php_time php(i+1)];
end  

% Grafica Cantidad de solicitudes vs Tiempo en completar todas
figure;
plot(seq_req,seq_time,'-ro', 'Color', 'red');
hold on;
plot(hp_req,hp_time,'-ro', 'Color', 'magenta');
plot(php_req,php_time,'-ro', 'Color', 'blue');
xlabel("Cantidad de solicitudes");
ylabel("Tiempo [s]");
title('Cantidad de solicitudes vs Tiempo en completar todas');
legend('Sequential', 'Heavy Process', 'Pre Heavy Process');
hold off;
