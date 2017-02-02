

enum DEVICE_STATE{
  DEVICE_BUSY,
  DEVICE_FREE,
  DEVICE_CONNECTED,
  DEVICE_DISCONNECTED
};

class Device{

  string name; 
  DEVICE_STATE state; 
  public: 
    
    void setName(char *name);
    void setState(DEVICE_STATE state);


    int addWork(IotRequest *req);
    /*
     * If the device is free, write to the tcp socket for the device.
     *    Set a maximum timer for the device to respond(say 3 seconds).
     *    If you dont get the reply send busy response and dequeue the request
     *    and process next if any. 
     *
     *    Else respond to the reply, pop queue and process next if any. 
     * Else set a timer to wait and return
     */
    int processNextWork();

    /*
     * store the request information in the device queue. Call addWork.
     * Try to process the work by calling processNextWork 
     */
    virtual int handleRequest(char *);
    

    static Device *formDeviceFromJson(char *msg);
}



