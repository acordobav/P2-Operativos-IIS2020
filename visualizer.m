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
ylabel("Tiempo en completar todas[s]");
title('Cantidad de solicitudes vs Tiempo en completar todas');
legend('Sequential', 'Heavy Process', 'Pre Heavy Process');
hold off;

% Calculo del promedio segun cantidad de solicitudes
seq_prom = [];
for i = 1:n1/2
  seq_prom = [seq_prom seq_time(i)/seq_req(i)];
end  

hp_prom = [];
for i = 1:n2/2
  hp_prom = [hp_prom hp_time(i)/hp_req(i)];
end  

php_prom = [];
for i = 1:n3/2
  php_prom = [php_prom php_time(i)/php_req(i)];
end  

% Grafica Cantidad de solicitudes vs Tiempo promedio por solicitud
figure;
plot(seq_req,seq_prom,'-ro', 'Color', 'red');
hold on;
plot(hp_req,hp_prom,'-ro', 'Color', 'magenta');
plot(php_req,php_prom,'-ro', 'Color', 'blue');
xlabel("Cantidad de solicitudes");
ylabel("Tiempo promedio por solicitud [s]");
title('Cantidad de solicitudes vs Tiempo promedio por solicitud');
legend('Sequential', 'Heavy Process', 'Pre Heavy Process');
hold off;

  
  
  
  
