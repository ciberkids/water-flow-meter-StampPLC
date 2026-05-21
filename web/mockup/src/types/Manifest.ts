import { z } from "zod";

export const manifestActionSchema = z.object({
  id: z.string(),
  name: z.string(),
  description: z.string().optional(),
  triggers: z.array(z.string()).optional()
});

export const manifestValueSchema = z.object({
  id: z.string(),
  name: z.string(),
  type: z.enum(["int", "float", "string", "bool"]),
  readOnly: z.boolean().default(true)
});

export const manifestSchema = z.object({
  actions: z.array(manifestActionSchema),
  values: z.array(manifestValueSchema)
});

export type ManifestAction = z.infer<typeof manifestActionSchema>;
export type ManifestValue = z.infer<typeof manifestValueSchema>;
export type Manifest = z.infer<typeof manifestSchema>;
