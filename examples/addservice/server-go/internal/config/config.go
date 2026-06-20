package config

import (
	"flag"
	"strings"
)

const (
	DefaultListen       = ":8080"
	DefaultBaseURL      = "http://localhost:8080"
	DefaultServicePath  = "/services/AddService"
	DefaultNamespace    = "http://example.com/addservice"
	DefaultWSDLTemplate = "templates/AddService.wsdl.tmpl"
)

type Config struct {
	Listen       string
	BaseURL      string
	ServicePath  string
	Namespace    string
	WSDLTemplate string
}

type Getenv func(string) string

func Load(args []string, getenv Getenv) (Config, error) {
	cfg := Config{
		Listen:       envOrDefault(getenv, "ADDSERVICE_LISTEN", DefaultListen),
		BaseURL:      envOrDefault(getenv, "ADDSERVICE_BASE_URL", DefaultBaseURL),
		ServicePath:  envOrDefault(getenv, "ADDSERVICE_SERVICE_PATH", DefaultServicePath),
		Namespace:    envOrDefault(getenv, "ADDSERVICE_NAMESPACE", DefaultNamespace),
		WSDLTemplate: envOrDefault(getenv, "ADDSERVICE_WSDL_TEMPLATE", DefaultWSDLTemplate),
	}

	flags := flag.NewFlagSet("addservice", flag.ContinueOnError)
	flags.StringVar(&cfg.Listen, "listen", cfg.Listen, "HTTP listen address")
	flags.StringVar(&cfg.BaseURL, "base-url", cfg.BaseURL, "public service base URL")
	flags.StringVar(&cfg.ServicePath, "service-path", cfg.ServicePath, "SOAP service path")
	flags.StringVar(&cfg.Namespace, "namespace", cfg.Namespace, "SOAP target namespace")
	flags.StringVar(&cfg.WSDLTemplate, "wsdl-template", cfg.WSDLTemplate, "WSDL template path")
	if err := flags.Parse(args); err != nil {
		return Config{}, err
	}

	cfg.BaseURL = strings.TrimRight(cfg.BaseURL, "/")
	if !strings.HasPrefix(cfg.ServicePath, "/") {
		cfg.ServicePath = "/" + cfg.ServicePath
	}
	return cfg, nil
}

func (c Config) Endpoint() string {
	return strings.TrimRight(c.BaseURL, "/") + c.ServicePath
}

func envOrDefault(getenv Getenv, name string, fallback string) string {
	if getenv == nil {
		return fallback
	}
	if value := strings.TrimSpace(getenv(name)); value != "" {
		return value
	}
	return fallback
}
