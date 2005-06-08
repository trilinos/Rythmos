
#include "Teuchos_RefCountPtr.hpp"
#include<iostream>

class Foo
{
  public:
    Foo() {};
    virtual ~Foo() {};
    virtual double getx() = 0;
};

class Bar : public virtual Foo
{
  public:
    Bar() {};
    ~Bar() {};
    double getx() { return(x_); };
    void setx(double x) { x_ = x; };
  protected:
    double x_;
};

class PrintFoo
{
  public:
    PrintFoo() {};
    PrintFoo(Teuchos::RefCountPtr<Foo> &F) { F_ = F; };
    ~PrintFoo() {};
    void setFoo(Teuchos::RefCountPtr<Foo> &F) { F = Teuchos::null; };
    void setConstFoo(const Teuchos::RefCountPtr<Foo> &F) { F_ = F; };
    void print()
      { std::cout << "x = " << F_->getx() << "!" << std::endl; };
  protected:
    Teuchos::RefCountPtr<Foo> F_;
};


// Print foo's x value when passed by RefCountPtr
void printFooDirect(Teuchos::RefCountPtr<Foo> F)
{
std::cout << "x = " << F->getx() << "!" << std::endl;
};
// Print foo's x value when passed by reference
void printFooDirect(Foo &F)
{
  std::cout << "x = " << F.getx() << "!" << std::endl;
};

int main(int argc, char *argv[])
{
  std::cout << "Output should be 5: " << std::endl;

  /*
  // This works because this is exactly the type of cast the compiler is expecting.
  Bar B;
  B.setx(5.0);
  printFooDirect(B);
  */

  /*
  // This works because the function printFooDirect is able to make the
  // RefCountPtr cast correctly somehow.
  Teuchos::RefCountPtr<Bar> B = Teuchos::rcp(new Bar);
  B->setx(5.0);
  printFooDirect(B);
  */
  
  /*
  // This fails because the PrintFoo constructor is not able to make the
  // RefCountPtr cast correctly.
  Teuchos::RefCountPtr<Bar> B = Teuchos::rcp(new Bar);
  B->setx(5.0);
  PrintFoo PF(B); // fails because PrintFoo takes RefCountPtr<Foo> not RefCountPtr<Bar>
  PF.print();
  */
  
  /*
  // This fails because PrintFoo.setFoo is not able to make the RefCountPtr
  // cast correctly either.
  Teuchos::RefCountPtr<Bar> B = Teuchos::rcp(new Bar);
  B->setx(5.0);
  PrintFoo PF;
  PF.setFoo(B);
  PF.print();
  */
  
  /*
  // This does work because the RefCountPtr on input to PrintFoo::setConstFoo
  // is a const reference.  This allows the compiler to do the inheritance
  // through the RefCountPtr (most likely due to Ross only implementing
  // inheritance for the const RefCountPtr constructor).  Contrary to my
  // intuition, the const in front of the RefCountPtr, simply says that the
  // RefCountPtr itself will not be swapped out with another inside this
  // routine.  It still allows the RefCountPtr to modify itself with the count,
  // if copied.  This is the correct way to pass RefCountPtr objects, by const
  // reference, provided you're not creating a new RefCountPtr and you want to
  // pass it out.
  Teuchos::RefCountPtr<Bar> B = Teuchos::rcp(new Bar);
  B->setx(5.0);
  PrintFoo PF;
  PF.setConstFoo(B);
  PF.print();
  */

  
  /*
  // This fails because B is cast (by RefCountPtr) as an object of type Foo which
  // doesn't have a member function setx.
  Teuchos::RefCountPtr<Foo> B = Teuchos::rcp(new Bar);
  B->setx(5.0); // fails because B is RefCountPtr<Foo> not RefCountPtr<Bar>
  PrintFoo PF(B);
  PF.print();
  */
  
  
  /*  
  // This does work because the constructor gets the right object and we
  // dynamic cast to get access to the new member function setx.
  Teuchos::RefCountPtr<Foo> B = Teuchos::rcp(new Bar);
  (Teuchos::rcp_dynamic_cast<Bar>(B))->setx(5.0);
  PrintFoo PF(B);
  PF.print();
  */  

  /*
  // This does work because we've made the correct RefCountPtr from a
  // reference.  In this case, the RefCountPtr is not managing the object, so
  // if the function that created the reference goes out of scope, then the
  // RefCountPtr will be pointing at garbage.
  Bar B;
  B.setx(5.0);
  Teuchos::RefCountPtr<Foo> F = Teuchos::rcp( &B, false );
  PrintFoo PF(F);
  PF.print();
  */

  
  /*
  // This does work because everything is of the right type, and we extracted a
  // reference to the object from the RefCountPtr, so when this routine goes
  // out of scope the reference goes away harmlessly.
  Teuchos::RefCountPtr<Foo> F = Teuchos::rcp(new Bar);
  Bar &B = *Teuchos::rcp_dynamic_cast<Bar>(F);
  B.setx(5.0);
  PrintFoo PF(F);
  PF.print();
  */

  // This does not work.  It compiles, but has a segfault at runtime because
  // the setFoo member function takes a nonconst reference to the RefCountPtr
  // and therefore is allowed to change the variable on exit, in this case
  // making it null, so that when it tries to print, it gets a null pointer to
  // the value.  So using const as in setConstFoo, does not allow the routine
  // to change the variable passed in, which would result in a compile error if
  // we tried to change it.  
  Teuchos::RefCountPtr<Foo> F = Teuchos::rcp(new Bar);
  Bar &B = *Teuchos::rcp_dynamic_cast<Bar>(F);
  B.setx(5.0);
  PrintFoo PF;
  PF.setFoo(F);
  PF.print();
  
  return 0;
}
