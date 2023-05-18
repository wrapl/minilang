.. include:: <isonum.txt>

.. include:: <isoamsa.txt>

.. include:: <isotech.txt>

polynomial
==========

.. rst-class:: mini-api

:mini:`fun polynomial::roots(Coeffs: list): list[complex]`
   Returns the roots of the single variable polynomial :math:`Coeff_{0} + Coeff_{1}x + Coeff_{2}x^{2} + ...`. If the degree is less than 5,  the relevant formula is used to calculate the roots,  otherwise the roots are estimated using an iterative process.

   .. code-block:: mini

      polynomial::roots([2, -3, 1]) :> [1, 2]


:mini:`meth (A: number) * (B: polynomial::rational): polynomial::rational`
   Returns :mini:`A * B`.


:mini:`meth (A: number) * (B: polynomial): polynomial`
   Returns :mini:`A * B`.


:mini:`meth (A: number) + (B: polynomial::rational): polynomial::rational`
   Returns :mini:`A + B`.


:mini:`meth (A: number) + (B: polynomial): polynomial`
   Returns :mini:`A + B`.


:mini:`meth (A: number) - (B: polynomial::rational): polynomial::rational`
   Returns :mini:`A - B`.


:mini:`meth (A: number) - (B: polynomial): polynomial`
   Returns :mini:`A - B`.


:mini:`meth (A: number) / (B: polynomial::rational): polynomial::rational`
   Returns :mini:`A / B`.


:mini:`meth (A: number) / (B: polynomial): polynomial::rational`
   Returns :mini:`A / B`.


:mini:`type polynomial < function`
   A polynomial with numeric (real or complex) coefficients.
   Calling a polynomial with named arguments returns the result of substituting the named variables with the corresponding values.


:mini:`meth polynomial(Var: string): polynomial`
   Returns the polynomial corresponding to the variable :mini:`Var`.

   .. code-block:: mini

      let X := polynomial("x"), Y := polynomial("y") :> y
      let P := (X - Y) ^ 4 :> x⁴ - 4x³y + 6x²y² - 4xy³ + y⁴
      P(y is 3) :> x⁴ - 12x³ + 54x² - 108x + 81


:mini:`meth (A: polynomial) * (B: number): polynomial`
   Returns :mini:`A * B`.


:mini:`meth (A: polynomial) * (B: polynomial::rational): polynomial::rational`
   Returns :mini:`A * B`.


:mini:`meth (A: polynomial) * (B: polynomial): polynomial`
   Returns :mini:`A * B`.


:mini:`meth (A: polynomial) + (B: number): polynomial`
   Returns :mini:`A + B`.


:mini:`meth (A: polynomial) + (B: polynomial::rational): polynomial::rational`
   Returns :mini:`A + B`.


:mini:`meth (A: polynomial) + (B: polynomial): polynomial`
   Returns :mini:`A + B`.


:mini:`meth (A: polynomial) - (B: number): polynomial`
   Returns :mini:`A - B`.


:mini:`meth (A: polynomial) - (B: polynomial::rational): polynomial::rational`
   Returns :mini:`A - B`.


:mini:`meth (A: polynomial) - (B: polynomial): polynomial`
   Returns :mini:`A - B`.


:mini:`meth (A: polynomial) / (B: number): polynomial`
   Returns :mini:`A / B`.


:mini:`meth (A: polynomial) / (B: polynomial::rational): polynomial::rational`
   Returns :mini:`A / B`.


:mini:`meth (A: polynomial) / (B: polynomial): polynomial::rational`
   Returns :mini:`A / B`.


:mini:`meth (A: polynomial) ^ (B: integer): polynomial`
   Returns :mini:`A ^ B`.


:mini:`meth (Poly: polynomial):coeff(Var: string, Degree: integer): number | polynomial`
   Returns the coefficient of :mini:`Var ^ Degree` in :mini:`Poly`.

   .. code-block:: mini

      let X := polynomial("x")
      (X ^ 2 + (3 * X) + 2):coeff("x", 1) :> 3


:mini:`meth (Poly: polynomial):d(Var: string): number | polynomial`
   Returns the derivative of :mini:`Poly` w.r.t. :mini:`Var`.

   .. code-block:: mini

      let X := polynomial("x")
      (X ^ 2 + (3 * X) + 2):d("x") :> 2x + 3


:mini:`meth (Poly: polynomial):degree(Var: string): integer`
   Returns the highest degree of :mini:`Var` in :mini:`Poly`.

   .. code-block:: mini

      let X := polynomial("x")
      (X ^ 2 + (3 * X) + 2):degree("x") :> 2


:mini:`meth (Arg₁: polynomial):red(Arg₂: polynomial)`
   *TBD*


:mini:`meth (Arg₁: polynomial):spol(Arg₂: polynomial)`
   *TBD*


:mini:`meth (Buffer: string::buffer):append(Poly: polynomial)`
   Appends a representation of :mini:`Poly` to :mini:`Buffer`.


:mini:`type polynomial::rational`
   *TBD*


:mini:`meth (A: polynomial::rational) * (B: number): polynomial::rational`
   Returns :mini:`A * B`.


:mini:`meth (A: polynomial::rational) * (B: polynomial::rational): polynomial::rational`
   Returns :mini:`A * B`.


:mini:`meth (A: polynomial::rational) * (B: polynomial): polynomial::rational`
   Returns :mini:`A * B`.


:mini:`meth (A: polynomial::rational) + (B: number): polynomial::rational`
   Returns :mini:`A + B`.


:mini:`meth (A: polynomial::rational) + (B: polynomial::rational): polynomial::rational`
   Returns :mini:`A + B`.


:mini:`meth (A: polynomial::rational) + (B: polynomial): polynomial::rational`
   Returns :mini:`A + B`.


:mini:`meth (A: polynomial::rational) - (B: number): polynomial::rational`
   Returns :mini:`A - B`.


:mini:`meth (A: polynomial::rational) - (B: polynomial::rational): polynomial::rational`
   Returns :mini:`A - B`.


:mini:`meth (A: polynomial::rational) - (B: polynomial): polynomial::rational`
   Returns :mini:`A - B`.


:mini:`meth (A: polynomial::rational) / (B: number): polynomial::rational`
   Returns :mini:`A / B`.


:mini:`meth (A: polynomial::rational) / (B: polynomial::rational): polynomial::rational`
   Returns :mini:`A / B`.


:mini:`meth (A: polynomial::rational) / (B: polynomial): polynomial::rational`
   Returns :mini:`A / B`.


:mini:`meth (Buffer: string::buffer):append(Poly: polynomial::rational)`
   Appends a representation of :mini:`Poly` to :mini:`Buffer`.


