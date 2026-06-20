package main

import (
	"log"
	"net/http"
	"os"
	"strings"

	"addservice/internal/addservice"
	"addservice/internal/config"
)

func main() {
	cfg, err := config.Load(os.Args[1:], os.Getenv)
	if err != nil {
		log.Fatal(err)
	}

	wsdl, err := addservice.RenderWSDL(cfg.WSDLTemplate, addservice.WSDLData{
		TargetNamespace: cfg.Namespace,
		Endpoint:        cfg.Endpoint(),
		SoapAction:      strings.TrimRight(cfg.Namespace, "/") + "/Add",
	})
	if err != nil {
		log.Fatal(err)
	}

	mux := http.NewServeMux()
	mux.Handle(cfg.ServicePath, addservice.NewHandler(addservice.HandlerConfig{
		Namespace: cfg.Namespace,
		WSDL:      wsdl,
	}))

	log.Printf("AddService listening on %s", cfg.Endpoint())
	log.Printf("WSDL available at %s?wsdl", cfg.Endpoint())
	log.Fatal(http.ListenAndServe(cfg.Listen, mux))
}
