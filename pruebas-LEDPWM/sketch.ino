int led = 2;
int brillo, brilloMinimo = 0;
int fade = 5;
int espera = 50;
int brilloMaximo = 255;

void setup() {
  pinMode(2, OUTPUT);
}

void loop() {
  analogWrite(led, brillo);

  brillo = brillo + fade;

  if (brillo <= brilloMinimo || brillo >= brilloMaximo) {
   fade = -fade;
 } 

  delay(espera); // this speeds up the simulation
}
