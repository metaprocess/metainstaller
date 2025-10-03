import { useState } from 'react'
import { useQuery } from '@tanstack/react-query'
import { X, FileText, Server, Terminal, Eye, Clock, HardDrive } from 'lucide-react'
import { DockerApiService } from '@/services/api'
import { Button, Card, CardHeader, CardTitle, CardContent, Badge, Alert, AlertDescription } from '@/components/ui'
import type { ProjectInfo } from '@/types/api'

interface ProjectDetailsProps {
  project: ProjectInfo
  onClose: () => void
}

export const ProjectDetails = ({ project, onClose }: ProjectDetailsProps) => {
  const [activeTab, setActiveTab] = useState<'info' | 'services' | 'logs'>('info')
  const [selectedService, setSelectedService] = useState<string>('')

  const { data: servicesData } = useQuery({
    queryKey: ['project-services', project.name],
    queryFn: () => DockerApiService.getProjectServices(project.name),
    enabled: activeTab === 'services',
  })

  const { data: logsData, isLoading: logsLoading } = useQuery({
    queryKey: ['project-logs', project.name, selectedService],
    queryFn: () => DockerApiService.getProjectLogs(project.name, selectedService),
    enabled: activeTab === 'logs',
    refetchInterval: 5000,
  })

  const { data: statusData } = useQuery({
    queryKey: ['project-status', project.name],
    queryFn: () => DockerApiService.getProjectStatus(project.name),
    // refetchInterval: 3000,
  })

  const services = servicesData?.services || []

  const getStatusBadge = () => {
    const status = statusData?.status || 'unknown'
    switch (status.toLowerCase()) {
      case 'running':
        return <Badge className="bg-green-100 text-green-800">Running</Badge>
      case 'stopped':
        return <Badge className="bg-red-100 text-red-800">Stopped</Badge>
      case 'ready':
        return <Badge className="bg-blue-100 text-blue-800">Ready</Badge>
      default:
        return <Badge className="bg-gray-100 text-gray-800">{status}</Badge>
    }
  }

  return (
    <div className="fixed inset-0 bg-black bg-opacity-50 flex items-center justify-center z-50">
      <div className="bg-white rounded-lg shadow-xl max-w-4xl w-full max-h-[90vh] overflow-auto mx-4">
        <div className="flex items-center justify-between p-6 border-b">
          <div>
            <h2 className="text-xl font-semibold">{project.name}</h2>
            <div className="flex items-center gap-2 mt-1">
              {getStatusBadge()}
              <span className="text-sm text-gray-500">{project.status_message}</span>
            </div>
          </div>
          <Button variant="ghost" size="sm" onClick={onClose}>
            <X className="h-4 w-4" />
          </Button>
        </div>

        <div className="border-b">
          <nav className="flex space-x-8 px-6">
            {[
              { id: 'info', label: 'Information', icon: FileText },
              { id: 'services', label: 'Services', icon: Server },
              { id: 'logs', label: 'Logs', icon: Terminal },
            ].map(({ id, label, icon: Icon }) => (
              <button
                key={id}
                onClick={() => setActiveTab(id as typeof activeTab)}
                className={`flex items-center gap-2 py-4 px-1 border-b-2 font-medium text-sm transition-colors ${
                  activeTab === id
                    ? 'border-blue-500 text-blue-600'
                    : 'border-transparent text-gray-500 hover:text-gray-700'
                }`}
              >
                <Icon className="h-4 w-4" />
                {label}
              </button>
            ))}
          </nav>
        </div>

        <div className="p-6">
          {activeTab === 'info' && (
            <div className="space-y-6">
              <div className="grid grid-cols-2 gap-6">
                <Card>
                  <CardHeader>
                    <CardTitle className="text-base">Project Details</CardTitle>
                  </CardHeader>
                  <CardContent className="space-y-3">
                    <div>
                      <span className="text-sm font-medium text-gray-500">Name</span>
                      <p className="font-mono text-sm">{project.name}</p>
                    </div>
                    <div>
                      <span className="text-sm font-medium text-gray-500">Created</span>
                      <p className="text-sm flex items-center gap-2">
                        <Clock className="h-3 w-3 text-gray-400" />
                        {new Date(project.created_time).toLocaleString()}
                      </p>
                    </div>
                    <div>
                      <span className="text-sm font-medium text-gray-500">Last Modified</span>
                      <p className="text-sm flex items-center gap-2">
                        <Clock className="h-3 w-3 text-gray-400" />
                        {new Date(project.last_modified).toLocaleString()}
                      </p>
                    </div>
                    <div>
                      <span className="text-sm font-medium text-gray-500">Loaded</span>
                      <div className="text-sm">
                        {project.is_loaded ? (
                          <Badge className="bg-green-100 text-green-800">Yes</Badge>
                        ) : (
                          <Badge className="bg-red-100 text-red-800">No</Badge>
                        )}
                      </div>
                    </div>
                    <div>
                      <span className="text-sm font-medium text-gray-500">Running</span>
                      <div className="text-sm">
                        {project.is_running ? (
                          <Badge className="bg-green-100 text-green-800">Yes</Badge>
                        ) : (
                          <Badge className="bg-red-100 text-red-800">No</Badge>
                        )}
                      </div>
                    </div>
                  </CardContent>
                </Card>

                <Card>
                  <CardHeader>
                    <CardTitle className="text-base">File Paths</CardTitle>
                  </CardHeader>
                  <CardContent className="space-y-3">
                    <div>
                      <span className="text-sm font-medium text-gray-500">Archive Path</span>
                      <p className="font-mono text-xs bg-gray-100 p-2 rounded break-all">
                        {project.archive_path}
                      </p>
                    </div>
                    <div>
                      <span className="text-sm font-medium text-gray-500">Extracted Path</span>
                      <p className="font-mono text-xs bg-gray-100 p-2 rounded break-all">
                        {project.extracted_path}
                      </p>
                    </div>
                    <div>
                      <span className="text-sm font-medium text-gray-500">Compose File</span>
                      <p className="font-mono text-xs bg-gray-100 p-2 rounded break-all">
                        {project.compose_file_path}
                      </p>
                    </div>
                  </CardContent>
                </Card>
              </div>

              <Card>
                <CardHeader>
                  <CardTitle className="text-base flex items-center gap-2">
                    <HardDrive className="h-4 w-4" />
                    Required Images ({project.required_images.length})
                  </CardTitle>
                </CardHeader>
                <CardContent>
                  <div className="space-y-2 max-h-32 overflow-y-auto">
                    {project.required_images.map((image, index) => (
                      <div key={index} className="flex items-center gap-2">
                        <div className="w-2 h-2 bg-blue-500 rounded-full"></div>
                        <code className="bg-gray-100 px-2 py-1 rounded text-sm">{image}</code>
                      </div>
                    ))}
                  </div>
                </CardContent>
              </Card>

              {project.dependent_files.length > 0 && (
                <Card>
                  <CardHeader>
                    <CardTitle className="text-base flex items-center gap-2">
                      <FileText className="h-4 w-4" />
                      Dependent Files ({project.dependent_files.length})
                    </CardTitle>
                  </CardHeader>
                  <CardContent>
                    <div className="space-y-2 max-h-32 overflow-y-auto">
                      {project.dependent_files.map((file, index) => (
                        <div key={index} className="flex items-center gap-2">
                          <div className="w-2 h-2 bg-amber-500 rounded-full"></div>
                          <code className="bg-gray-100 px-2 py-1 rounded text-sm">{file}</code>
                        </div>
                      ))}
                    </div>
                  </CardContent>
                </Card>
              )}
            </div>
          )}

          {activeTab === 'services' && (
            <div className="space-y-4">
              {services.length === 0 ? (
                <Alert>
                  <AlertDescription>
                    No services found for this project.
                  </AlertDescription>
                </Alert>
              ) : (
                <Card>
                  <CardHeader>
                    <CardTitle className="text-base">Services ({services.length})</CardTitle>
                  </CardHeader>
                  <CardContent>
                    <div className="grid grid-cols-2 gap-3">
                      {services.map((service, index) => (
                        <div
                          key={index}
                          className="flex items-center gap-3 p-3 border rounded-lg hover:bg-gray-50"
                        >
                          <Server className="h-5 w-5 text-blue-600" />
                          <div>
                            <p className="font-medium">{service}</p>
                            <p className="text-xs text-gray-500">Docker Compose Service</p>
                          </div>
                        </div>
                      ))}
                    </div>
                  </CardContent>
                </Card>
              )}
            </div>
          )}

          {activeTab === 'logs' && (
            <div className="space-y-4">
              <div className="flex items-center gap-2">
                <span className="text-sm font-medium">Service:</span>
                <select
                  value={selectedService}
                  onChange={(e) => setSelectedService(e.target.value)}
                  className="px-3 py-1 border rounded text-sm"
                >
                  <option value="">All services</option>
                  {services.map((service) => (
                    <option key={service} value={service}>
                      {service}
                    </option>
                  ))}
                </select>
              </div>

              <Card>
                <CardHeader>
                  <CardTitle className="text-base flex items-center gap-2">
                    <Terminal className="h-4 w-4" />
                    Logs {selectedService && `(${selectedService})`}
                  </CardTitle>
                </CardHeader>
                <CardContent>
                  {logsLoading ? (
                    <div className="flex justify-center py-4">
                      <div className="animate-spin rounded-full h-6 w-6 border-b-2 border-blue-600"></div>
                    </div>
                  ) : logsData?.logs ? (
                    <div className="bg-black text-green-400 font-mono text-sm p-4 rounded max-h-80 overflow-y-auto">
                      <pre className="whitespace-pre-wrap">{logsData.logs}</pre>
                    </div>
                  ) : (
                    <Alert>
                      <AlertDescription>
                        No logs available for this project.
                      </AlertDescription>
                    </Alert>
                  )}
                </CardContent>
              </Card>
            </div>
          )}
        </div>

        <div className="flex justify-end p-6 border-t">
          <Button onClick={onClose}>Close</Button>
        </div>
      </div>
    </div>
  )
}
