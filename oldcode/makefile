all: 
	-cd so-commons-library && $(MAKE) install
	-cd parser && $(MAKE) install
	-cd ML-libraries && $(MAKE) install
	-cd Consola && $(MAKE) install
	-cd CPU && $(MAKE) all
	-cd Nucleo && $(MAKE) all
	-cd UMC && $(MAKE) all
	-cd Swap && $(MAKE) all

clean:
	-cd so-commons-library && $(MAKE) clean
	-cd so-commons-library && $(MAKE) uninstall
	-cd parser && $(MAKE) clean
	-cd parser && $(MAKE) uninstall
	-cd ML-libraries && $(MAKE) clean
	-cd ML-libraries && $(MAKE) uninstall
	-cd Consola && $(MAKE) clean
	-cd CPU && $(MAKE) clean
	-cd Nucleo && $(MAKE) clean
	-cd UMC && $(MAKE) clean
	-cd Swap && $(MAKE) clean
