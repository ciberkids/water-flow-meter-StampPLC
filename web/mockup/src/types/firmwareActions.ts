
export interface FirmwareActionDefinition {
  id: string;
  name: string;
  description?: string;
  triggers?: string[];
  // validations?: ... ?
}

export interface FirmwareValueDefinition {
  id: string;
  type?: string;
  unit?: string;
  register?: number;
  description?: string;
  readOnly?: boolean;
}

export interface FirmwareActionManifest {
  actions: FirmwareActionDefinition[];
  values?: FirmwareValueDefinition[];
}

