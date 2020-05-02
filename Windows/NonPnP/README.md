Introduction:
  This is a Non PnP driver, also called as software driver, it is not associated with any device. In simple, it is a software runs in kernel mode and provides services to the user application.
  
  In this, user application sends a name for example "Manoj" to the driver and returns the message "Hello Manoj!, greetings from NonPnP driver.".

Instructions:
  1. Open NonPnP.sln and build the solution.
  2. Run NonPnPDrv.sys service using osrloader.exe (Browse driver, Register Service, Start Service).
  3. Run NonPnPApp.exe.
  4. Stop NonPnpDrv.sys service using osrloader.exe (Stop Service, Unregister Service).
  
  Input:
  Manoj
  
  Output:
  "Hello Manoj!, greetings from NonPnP driver."
