clc; clear; close all;

% Lectura de los archivos de reportes
child_php = load("-ascii","php_child_report.txt");

% Tamano de la cantidad de muestras
[~,n4] = size(child_php);

% Grafica Distribucion de carga servidor Pre Heavy Process
figure
x = 1:n4;
w1 = 0.5;
bar(x, child_php, 'c');
title("Distribución de carga entre procesos - Servidor Pre Heavy Process");
grid on;
xlabel('Identificador del proceso');
ylabel('Cantidad de solicitudes atendidas');