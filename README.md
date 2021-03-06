#Atracados
##NPI-Kinect1

Este proyecto ha sido realizado en Granada, 30/10/2015 por:
* Jose Antonio Jiménez Montañez
* Carlos Cobos Suárez

###Descripción
En este proyecto se ha elaborado un juego que consiste en el que el jugador tiene que ir capturando monedas que salen aleatoriamente en su entorno.
Para ello, al comenzar el juego, el jugador tiene que empezar en una posición concreta que se indica en el juego a través de una animación si se presiona la tecla "h" en el juego.

Conforme el juego avanza, van a ir saliendo más y más monedas. Si el jugador captura determinadas monedas va a disponer de un bonus que se activará a través de un gesto. Este bonus, capturará todas las monedas que están en la pantalla para que el jugador obtenga más puntuación.

###Solución diseñada
Para desarrollar este proyecto, hemos optado por programar en C++ añandiéndole las librerías de Kinect y OpenCV.
Las librerías de Kinect han sido añadidas ya que este juego ha sido desarrollado para este dispositivo por lo que es necesario disponer de una API para interactuar con este.
Para el dibujo de las imágenes en pantalla así como las comprobaciones del esqueleto para las posturas y la interacción con las monedas hemos decidido usar OpenCV.

###Errores frecuentes
* Dificultad para concatenar matrices si estas no son iguales en OpenCV
* Tipo de dato para la profundidad en la función de transformación de coordenadas de Kinect a ventana
* Calibrar el esquleto tomado de la API de Kinect a mano
* Escritura y lectura de estructuras de puntos de Kinect en ficheros
* Control del entorno para una buena captura de Kinect

###Destacados
Para hacer las ilustración del manual de la primera posición, hemos capturado el esqueleto mientras estábamos haciendo la posición y hemos guardado la transición del esqueleto en un fichero. La ayuda de la primera posición es la reproducción de esta captura por lo que se verá como el esqueleto hace la postura.

###Referencias
Al trabajar con una API nueva para nosotros como Kinect hemos tenido que consultar la información oficial de Microsoft:
* https://msdn.microsoft.com/en-us/library/hh855364.aspx

Asi como manuales y tutoriales de fuentes externas:
* https://dharuiz.wordpress.com/
* http://acodigo.blogspot.com.es/2013/05/dibujar-formas-y-texto.html

También para manejar OpenCV hemos recurrido a su documentación:
* http://docs.opencv.org/3.0.0/