import { useDockerInfo, useDockerServiceStatus, useSystemIntegrationStatus, useContainers, useImages } from '@/hooks/useApi'
import { Card, CardContent, CardDescription, CardHeader, CardTitle } from '@/components/ui'
import { Badge } from '@/components/ui'
import { Container, HardDrive, CheckCircle, XCircle, AlertCircle, Server } from 'lucide-react'
/* import { DockerApiService } from '@/services/api'
import { useEffect } from 'react' */

const Dashboard = () => {
  const { data: dockerInfo } = useDockerInfo()
  const { data: serviceStatus } = useDockerServiceStatus(true) // Only poll on dashboard
  const { data: integrationStatus } = useSystemIntegrationStatus()
  const { data: containers } = useContainers(true)
  const { data: images } = useImages()

  /* useEffect(() => {
    DockerApiService.getDockerServiceStatus();
  }, []) */
  

  const getStatusIcon = (status: boolean) => {
    return status ? (
      <CheckCircle className="h-5 w-5 text-green-500" />
    ) : (
      <XCircle className="h-5 w-5 text-red-500" />
    )
  }

  const getServiceStatusBadge = (running: boolean) => {
    return (
      <Badge variant={running ? 'success' : 'destructive'}>
        {running ? 'Running' : 'Stopped'}
      </Badge>
    )
  }

  return (
    <div className="space-y-6">
      <div>
        <h1 className="text-3xl font-bold">META INSTALLER Dashboard</h1>
        <p className="text-gray-600 mt-2">Monitor and manage Services</p>
      </div>

      <div className="grid gap-6 md:grid-cols-2 lg:grid-cols-4">
        <Card>
          <CardHeader className="flex flex-row items-center justify-between space-y-0 pb-2">
            <CardTitle className="text-sm font-medium">Main Service</CardTitle>
            <Server className="h-4 w-4 text-gray-500" />
          </CardHeader>
          <CardContent>
            <div className="flex items-center space-x-2">
              {getStatusIcon(dockerInfo?.installed || false)}
              <span className="text-2xl font-bold">
                {dockerInfo?.installed ? 'Installed' : 'Not Installed'}
              </span>
            </div>
            {dockerInfo?.version && (
              <p className="text-xs text-gray-500 mt-1">Version {dockerInfo.version}</p>
            )}
          </CardContent>
        </Card>

        <Card>
          <CardHeader className="flex flex-row items-center justify-between space-y-0 pb-2">
            <CardTitle className="text-sm font-medium">Service Status</CardTitle>
            <AlertCircle className="h-4 w-4 text-gray-500" />
          </CardHeader>
          <CardContent>
            <div className="flex items-center space-x-2">
              {getServiceStatusBadge(serviceStatus?.running || false)}
            </div>
            <p className="text-xs text-gray-500 mt-1">
              {serviceStatus?.running ? 'Service is active' : 'Service is inactive'}
            </p>
          </CardContent>
        </Card>

        <Card>
          <CardHeader className="flex flex-row items-center justify-between space-y-0 pb-2">
            <CardTitle className="text-sm font-medium">Containers</CardTitle>
            <Container className="h-4 w-4 text-gray-500" />
          </CardHeader>
          <CardContent>
            <div className="text-2xl font-bold">{containers?.containers.length || 0}</div>
            <p className="text-xs text-gray-500 mt-1">
              Total containers
            </p>
          </CardContent>
        </Card>

        <Card>
          <CardHeader className="flex flex-row items-center justify-between space-y-0 pb-2">
            <CardTitle className="text-sm font-medium">Images</CardTitle>
            <HardDrive className="h-4 w-4 text-gray-500" />
          </CardHeader>
          <CardContent>
            <div className="text-2xl font-bold">{images?.images.length || 0}</div>
            <p className="text-xs text-gray-500 mt-1">
              Total images
            </p>
          </CardContent>
        </Card>
      </div>

      <div className="grid gap-6 md:grid-cols-2">
        <Card>
          <CardHeader>
            <CardTitle>System Integration</CardTitle>
            <CardDescription>Docker system integration status</CardDescription>
          </CardHeader>
          <CardContent>
            <div className="space-y-3">
              <div className="flex items-center justify-between">
                <span className="text-sm">Docker Running</span>
                {getStatusIcon(integrationStatus?.docker_running || false)}
              </div>
              <div className="flex items-center justify-between">
                <span className="text-sm">Socket Enabled</span>
                {getStatusIcon(integrationStatus?.docker_socket_enabled || false)}
              </div>
              <div className="flex items-center justify-between">
                <span className="text-sm">Group Exists</span>
                {getStatusIcon(integrationStatus?.docker_group_exists || false)}
              </div>
              <div className="flex items-center justify-between">
                <span className="text-sm">Service Enabled</span>
                {getStatusIcon(integrationStatus?.docker_service_enabled || false)}
              </div>
              <div className="flex items-center justify-between">
                <span className="text-sm">Systemd Services</span>
                {getStatusIcon(integrationStatus?.systemd_services_installed || false)}
              </div>
              <div className="flex items-center justify-between">
                <span className="text-sm">Binaries Installed</span>
                {getStatusIcon(integrationStatus?.binaries_installed || false)}
              </div>
              <div className="flex items-center justify-between pt-2 border-t">
                <span className="text-sm font-medium">Fully Integrated</span>
                {getStatusIcon(integrationStatus?.fully_integrated || false)}
              </div>
            </div>
          </CardContent>
        </Card>

        <Card>
          <CardHeader>
            <CardTitle>Quick Actions</CardTitle>
            <CardDescription>Common Docker management tasks</CardDescription>
          </CardHeader>
          <CardContent>
            <div className="grid gap-2">
              <a 
                href="/installation" 
                className="p-3 text-sm border rounded-lg hover:bg-gray-50 transition-colors"
              >
                Install/Uninstall Docker
              </a>
              <a 
                href="/service" 
                className="p-3 text-sm border rounded-lg hover:bg-gray-50 transition-colors"
              >
                Manage Docker Service
              </a>
              <a 
                href="/projects" 
                className="p-3 text-sm border rounded-lg hover:bg-gray-50 transition-colors"
              >
                MetaProjects
              </a>
              <a 
                href="/containers" 
                className="p-3 text-sm border rounded-lg hover:bg-gray-50 transition-colors"
              >
                View Containers
              </a>
              <a 
                href="/images" 
                className="p-3 text-sm border rounded-lg hover:bg-gray-50 transition-colors"
              >
                Manage Images
              </a>
            </div>
          </CardContent>
        </Card>
      </div>
    </div>
  )
}

export default Dashboard
